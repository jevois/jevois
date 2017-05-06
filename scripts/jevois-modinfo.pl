#!/usr/bin/perl

# USAGE jevois-modinfo.pl <module.C>
# writes out modinfo.yaml

use Text::Wrap;
use Cwd 'abs_path';
use File::Find;
use File::Basename;
use File::Spec::Unix;
use HTML::Entities;
#use strict;

my $tmpdirname = "/tmp/jevois-modinfotemp$$".time();

# Parse out the module header filename from the command line
my $num_args = $#ARGV + 1;
if ($num_args != 1) {
  print STDERR "Usage: jevois-modinfo.pl <file.C|filestem>\n";
  exit(-1);
}
my $inputFilename = $ARGV[0];

# If we were not given an extension (as is the case when using 'make moduledoc'), try to figure it out:
my @ifn = split(/\./, $inputFilename); my $inputExt = pop(@ifn);
my @extensions = qw/ H hh hpp C cc cpp /; my $exts = '^(' . join('|', @extensions) . ')$'; 
if ($inputExt !~ m/$exts/)
{ foreach my $e (@extensions) { if (-f "$inputFilename.$e") { $inputFilename = "$inputFilename.$e"; last; } } }

# Make sure we have doxygen installed
my $doxypath = `which doxygen`;
if($doxypath eq "")
{
  print STDERR "Could not create modinfo.yaml: doxygen is not installed on this machine\n";
  exit(-1);
}

# Look for an icon.* file
my @iconFileArray = split("/", $inputFilename);
$iconFileArray[-1] = "icon.*";
my @iconFiles = glob(join("/", @iconFileArray));
my $iconFile = "";
if (length(@iconFiles) > 0) { $iconFile = (split("/", $iconFiles[0]))[-1]; }

# Create a temporary working directory:
`mkdir -p $tmpdirname`;

# Search the module header file for any ifdefs, and just blindly define them in the PREDEFINED doxygen tag
my $ifdefs = `grep ifdef $inputFilename`;
my @ifdeflist = (split("\n", $ifdefs));
my $ifdefnames = "";
foreach my $ifdef (@ifdeflist) { $ifdefnames= "$ifdefnames ". substr($ifdef, length("#ifdef ")); }

# Create a doxygen config file:
open  DOXYFILE, ">$tmpdirname/doxy.dxy" or die $!;
print DOXYFILE "INPUT                  = $tmpdirname/ $inputFilename\n";
print DOXYFILE "PREDEFINED 			   = $ifdefnames\n";
print DOXYFILE "OUTPUT_DIRECTORY       = $tmpdirname\n";
print DOXYFILE "RECURSIVE              = YES\n";
print DOXYFILE "GENERATE_PERLMOD       = YES\n";
print DOXYFILE "GENERATE_HTML          = YES\n";
print DOXYFILE "GENERATE_LATEX         = NO\n";
print DOXYFILE "QUIET                  = YES\n";
print DOXYFILE "WARNINGS               = NO\n";
print DOXYFILE "WARN_IF_UNDOCUMENTED   = NO\n";
print DOXYFILE "WARN_IF_DOC_ERROR      = NO\n";
print DOXYFILE "PERLMOD_LATEX          = NO\n";
print DOXYFILE "GENERATE_XML           = NO\n";
print DOXYFILE "EXTENSION_MAPPING      = H=C++ C=C++\n";
print DOXYFILE "EXTRACT_ALL            = YES\n";
print DOXYFILE "TEMPLATE_RELATIONS     = NO\n";
print DOXYFILE "ABBREVIATE_BRIEF       = NO\n";
print DOXYFILE "REPEAT_BRIEF           = NO\n";
print DOXYFILE "INHERIT_DOCS           = YES\n";
print DOXYFILE "AUTOLINK_SUPPORT       = YES\n"; # for cross-ref to jevois doc
print DOXYFILE "BUILTIN_STL_SUPPORT    = YES\n"; # for cross-ref to jevois doc
#print DOXYFILE "TAGFILES               = $ENV{'JEVOIS_SRC_ROOT'}/jevoisbase/doc/jevoisbase.tag=/basedoc\n";
#print DOXYFILE "TAGFILES               = $ENV{'JEVOIS_SRC_ROOT'}/jevois/doc/jevois.tag=/doc $ENV{'JEVOIS_SRC_ROOT'}/jevoisbase/doc/jevoisbase.tag=/basedoc\n";
print DOXYFILE "TAGFILES               = $ENV{'JEVOIS_SRC_ROOT'}/jevois/doc/jevois.tag=/doc\n";

# We here create some custom doxygen tags to allow code writers to input more manifest data in their doc:
my @dtags = qw/ email mainurl supporturl otherurl address copyright license distribution 
 restrictions videomapping subcomponents depends displayname modulecommand /;
#future use?: / recommends suggests conflicts replaces breaks provides /;
foreach my $kw (@dtags)
{ print DOXYFILE "ALIASES += \"$kw=\\xrefitem $kw \\\"$kw\\\" \\\"$kw\\\"\"\n"; }
close DOXYFILE;

##############################################################################################################
# Run doxygen to generate our PERLMOD file, and traverse the resulting structure to extract data from the docs
##############################################################################################################

`doxygen $tmpdirname/doxy.dxy`;

# Import our newly generated DoxyDocs.pl
#my $doxydocs = { };
require $tmpdirname . "/perlmod/DoxyDocs.pm";

my %docs = %$doxydocs;
my @classes = @{ $docs{classes} };
my %class = {}; #  %{ $classes[0] };

# Find the right class in the file by finding the one which inherits from jevois::Module. If there are multiple,
# find the one with the lowest levenshtein distance to the filename.
my $headerFileName = (split("/", $inputFilename))[-1];
$headerFileName =~ s/\..*//;
my $minDist = 100000000000000;
foreach my $tmpclassref (@classes) 
{
    my %tmpclass = %{$tmpclassref};
    my @bases = @{$tmpclass{base}};
    foreach my $baseref (@bases)
    {
        my %base = %{$baseref};
        
        if ($base{name} eq "jevois::Module" or $base{name} eq "Module")
        {
            my $distance = levenshtein($tmpclass{name}, $headerFileName);
            if ($distance < $minDist) { %class = %tmpclass; $minDist = $distance; }
        }
    }
}

##############################################################################################################
# Search for our custom doxygen tags
##############################################################################################################
my %tagdata;

my @pages = @{$docs{pages}};
for my $page (@pages)
{
    my %pageHash = %{$page};
    foreach my $tag (@dtags)
    {
        if ($pageHash{name} eq $tag) # this page is for that tag, parse it
        {
            my @doc = @{$pageHash{detailed}->{doc}};
            my @data;
            for my $docentry (@doc)
            {
                my %docentryHash = %{$docentry};
                if (($docentryHash{type} eq "text" or $docentryHash{type} eq "url")
                    and $docentryHash{content} !~ m/\s*Class\s*/)
                {
                    my @curr = split(/\s*,\s*/, trim($docentryHash{content}));
                    # skip empty fields:
                    foreach my $c (@curr) { if ($c !~ m/^\s+$/) { push(@data, $c); } }
                }
            }
            if ($tag eq 'modulecommand') { $tagdata{$tag} = join("|", @data); }
            else { $tagdata{$tag} = join(", ", @data); }
        }
    }
}

##############################################################################################################
# Extract the module name
##############################################################################################################
my $className = $class{name};

# strip any namespace:: from the classname:
$className =~ s/.+:://;

# If the displayname is not given explicitly, compute it from class name. Separate the CamelCase module class name into
# separate tokens, and recombine with spaces in between. If the last token is "Module", then just remove it.
my $displayName = $tagdata{'displayname'};

if ($displayName =~ m/^\s*$/)
{
    $displayName = $className; $displayName =~ s/_/ /g;
    my @displayNameSplit =
        $displayName =~ /[[:lower:]0-9]+|[[:upper:]0-9](?:[[:upper:]0-9]+|[[:lower:]0-9]*)(?=$|[[:upper:]0-9])/g;

    if ($displayNameSplit[-1] eq "Module") { pop(@displayNameSplit); }
    $displayName = "@displayNameSplit";
}

##############################################################################################################
# Extract the synopsis
##############################################################################################################
my @brief = @{$class{brief}{doc}};
my $synopsis = "";
foreach my $b (@brief)
{
  my %hash = %{$b};
  if (exists $hash{content}) { $synopsis = $hash{content}; }
}
$synopsis =~ s/'//g;
if ($synopsis eq "") { $synopsis = "This author is too lazy to write a synopsis!"; }

##############################################################################################################
# Extract the description and authors:
##############################################################################################################
my @detailed = @{$class{detailed}{doc}};
my $description = "";
my @authors;
foreach my $d (@detailed)
{
  my %hash = %{$d};
  if ($hash{type} eq "text") { $description = $description . $hash{content}; }
  elsif ($hash{type} eq "url") { $description = $description . $hash{content}; }
  elsif ($hash{type} eq "parbreak") { $description = $description . "<p>"; }
  
  # Extract the authors
  if (exists $hash{author})
  {
      my @authorsArray = @{$hash{author}};
      foreach my $authorHashRef (@authorsArray)
      {
          my %authorHash = %{$authorHashRef};
          if ($authorHash{type} eq "text") { push(@authors, trim($authorHash{content})); }
      }
  }
}

$description =~ s/'//g;
if ($description eq "") { $description = "This author is too lazy to write a description!"; }

##############################################################################################################
# Search for parameter declarations
##############################################################################################################
# parsing of JEVOIS_DECLARE_PARAMETER(...) is garbled in the perldoc output, so we just run the C++ pre-processor
my @modparams;
getParams($inputFilename, $className);

# now also include the parameters for all components, recursively:
my @subcomps;
my @sclist = `/bin/grep addSubComponent $inputFilename`;
foreach my $comp (@sclist) { my @tmp = split(/\<|\>/, $comp); push(@subcomps, $tmp[1]); }

foreach my $comp (@subcomps) {
    my $filename = `find ../../Components/ -name ${comp}.H`;
    chomp $filename;
    print STEDRR  "INCLUDING $filename\n";
    getParams($filename, $comp);

    # FIXME this is not recursive yet
}

##############################################################################################################
# OS-related and JEVOIS-related data:
##############################################################################################################
my $os = trim(`cat /etc/issue`);
my @ostmp = split(/\s+/, $os);
my $osname = $ostmp[0];
my $osver = $ostmp[1];

##############################################################################################################
# Dump out information in YAML format
##############################################################################################################
open OF, ">modinfo.yaml" || die "oops";
print OF "general: \n";
print OF "  displayname: $displayName \n";
print OF "  synopsis: $synopsis \n";
print OF "  description: > \n";
print OF "    " . wrap("","      ", $description). "\n";
print OF "\n";
#print OF "  keywords: \n";
#print OF join("\n", map {$_ = "    - $_"} (split(/\s*,\s*/, $tagdata{'keywords'}))) . "\n";
#print OF "\n";
#print OF "  jevoispkg: $tagdata{'jevoispkg'}\n";
#print OF "\n";

print OF "  parameters: \n";
foreach my $p (@modparams)
{
    my %paramHash = %$p;
    print OF "    - name: " .        $paramHash{name} .        "\n";
    print OF "      type: " .        $paramHash{type} .        "\n";
    print OF "      description: " . $paramHash{description} . "\n";
    print OF "      defval: "      . $paramHash{defval} . "\n";
    print OF "      validvals: "   . $paramHash{vvals} . "\n";
}
print OF "\n";

print OF "\n";
print OF "authorship: " .                     "\n";
print OF "  author: " . join(", ", @authors) . "\n";
print OF "  email: $tagdata{'email'}\n";
print OF "  mainurl: $tagdata{'mainurl'}\n";
print OF "  supporturl: $tagdata{'supporturl'}\n";
print OF "  otherurl: $tagdata{'otherurl'}\n";
print OF "  address: $tagdata{'address'}\n";
print OF "\n";
print OF "licensing: " .                      "\n";
print OF "  copyright: $tagdata{'copyright'}\n";
print OF "  license: $tagdata{'license'}\n";
print OF "  distribution: $tagdata{'distribution'}\n";
print OF "  restrictions: $tagdata{'restrictions'}\n";
print OF "\n";
print OF "os: " .                             "\n";
print OF "  name: $osname" .                  "\n";
print OF "  version: $osver" .                "\n";
print OF "  depends: $tagdata{'depends'}\n";
#print OF "  recommends: $tagdata{'recommends'}\n";
#print OF "  suggests: $tagdata{'suggests'}\n";
#print OF "  conflicts: $tagdata{'conflicts'}\n";
#print OF "  replaces: $tagdata{'replaces'}\n";
#print OF "  breaks: $tagdata{'breaks'}\n";
#print OF "  provides: $tagdata{'provides'}\n";
print OF "\n";
#print OF "package: " .                        "\n";
#print OF "  signature: " .                    "\n";
#print OF "  md5: " .                          "\n";
close OF;

##############################################################################################################
# Dump out information in HTML format
##############################################################################################################

# The detailed section in the perldoc is all broken down into elements which would be a pain to re-assemble. So we are
# going to use th ehtml output to get the html version of the detailed info. Because there are no good markers in there,
# we will assume that "author" is the first field that comes after the detailed info:
my $htmldescription; my $grabbing = 0;
open HTML, "$tmpdirname/html/class${className}.html" || die "oops";
while (my $line = <HTML>)
{
    if ($line =~ m/section author/) { $grabbing = 0; }
    if ($grabbing) { $htmldescription .= $line; }
    if ($line =~ m/Detailed Description/) { $grabbing = 1; }
}
close HTML;
 
my $fullhtml = 1; # set to 0 to remove the preamble

open OF, ">modinfo.html" || die "oops";

if ($fullhtml)
{
    print OF "<html><head>\n<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n";
    print OF "<META HTTP-EQUIV=\"Content-Language\" CONTENT=\"en-US\"><META NAME=\"robots\" CONTENT=\"index, follow\">\n";
    print OF "<META NAME=\"rating\" CONTENT=\"General\"><META NAME=\"distribution\" CONTENT=\"Global\">\n";
    print OF "<META NAME=\"revisit-after\" CONTENT=\"15 days\"><META NAME=\"author\" CONTENT=\"Laurent Itti, JeVois\">\n";
    print OF "<META NAME=\"description\" CONTENT=\"JeVois Smart Embedded Machine Vision Toolkit - module $className\">\n";

    print OF "<link href='http://fonts.googleapis.com/css?family=Open+Sans:300italic,400italic,600italic,700italic,800italic,400,300,600,700,800' rel='stylesheet' type='text/css'>\n";
#    print OF "<link rel=\"stylesheet\" href=\"/start/assets/plugins/bootstrap/css/bootstrap.min.css\">\n";
#    print OF "<link rel=\"stylesheet\" href=\"/start/assets/plugins/font-awesome/css/font-awesome.css\">\n";
#    print OF "<link rel=\"stylesheet\" href=\"/start/assets/plugins/prism/prism.css\">\n";
#    print OF "<link rel=\"stylesheet\" href=\"/start/assets/plugins/lightbox/dist/ekko-lightbox.min.css\">\n";
#    print OF "<link rel=\"stylesheet\" href=\"/start/assets/plugins/elegant_font/css/style.css\">\n";
#    print OF "<link id=\"theme-style\" rel=\"stylesheet\" href=\"/start/assets/css/styles.css\">\n";
#    print OF "<!--[if lt IE 9]>
#      <script src=\"https://oss.maxcdn.com/html5shiv/3.7.2/html5shiv.min.js\"></script>
#      <script src=\"https://oss.maxcdn.com/respond/1.4.2/respond.min.js\"></script>
#    <![endif]-->\n";

    print OF "<link rel=\"stylesheet\" type=\"text/css\" href=\"/modstyle.css\">\n";
    print OF "</head> <body>\n";
}

# The main table has only one column, we will place sub-tables when we need more columns:
print OF "<table class=modinfo><tr><td>\n";

# Header: icon, title, synopsis, author info
print OF "<table class=modinfotop><tr><td><a href=\"/moddoc/$className/modinfo.html\"><img src=\"/moddoc/$className/$iconFile\" width=48></a></td>\n";
print OF "<td valign=middle><table><tr><td class=modinfoname>$displayName</td></tr>\n";
print OF "<tr><td class=modinfosynopsis>$synopsis</td></tr></table></td></tr></table></td></tr>\n";
print OF "<tr><td><table class=modinfoauth width=100%><tr><td>By ". join(", ", @authors) . "</td><td>$tagdata{'email'}</td><td>$tagdata{'mainurl'}</td>";
print OF "<td>$tagdata{'license'}</td></tr></table></td></tr>\n";

# Video mappings: We need to clean them up first
print OF "<tr><td><table class=videomapping>\n";
my $vm = $tagdata{'videomapping'};
my @vm2 = split(/,/, $vm);
my @vmap; my $ii = 0; my $vvv = "";
foreach my $v (@vm2)
{
    if ($ii == 0) { $vvv = $v; $ii++; }
    else
    {
        if (substr($v, 1, 1) eq '#') { push(@vmap, "$vvv<em>$v</em>"); $ii = 0; }
        elsif ($ii == 2) { push(@vmap, $vvv); $vvv = $v; $ii = 1; }
        else { $vvv .= " $v"; $ii ++; }
    }
}

if ($vvv ne "") { push(@vmap, $vvv); }

foreach $v (@vmap) {
    $v =~ s/^\s+//; $v =~ s/\s+/\&nbsp\;/g;
    print OF "<tr><td class=videomapping><small><b>&nbsp;Video Mapping: &nbsp; </b></small><tt>$v</tt></td></tr>\n";
}
print OF "</table></td></tr>\n";

# description:
# note: doxygen opens a div but does not close it?
print OF "<tr><td class=modinfodesc><h2>Module Documentation</h2>$htmldescription</div></td></tr>\n";

# screenshots and videos:
print OF "<tr><td><table class=modinfoshots><tr>\n";
my @screenshots = glob("screenshot*.*");
my @videos = glob("video*.*");
if ($#screenshots == -1 && $#videos == -1) {
    print OF "<td>This module has no screenshots and no videos</td>\n";
} else {
    foreach my $sc (@screenshots) { print OF "<td><a href=\"$sc\"><img src=\"$sc\" width=128></a></td>\n"; }
    foreach my $vd (@videos) { print OF "<td><a href=\"$vd\">VIDEO</a></td>\n"; }
}
print OF "</tr></table></td></tr>";

# Custom commands:
my $cm = $tagdata{'modulecommand'};
if ($cm ne "")
{
    print OF "<tr><td><table class=modulecommand><tr><th class=modulecommand>Custom module commands</th></tr>\n";
    my @cm2 = split(/\|/, $cm);

    foreach $cm (@cm2) {
        print OF "<tr><td class=modulecommand>$cm</td></tr>\n";
    }
    print OF "</table></td></tr>\n";
}

# parameters:
print OF "<tr><td><table class=modinfopar><tr><th class=modinfopar>Parameter</th><th class=modinfopar>Type</th><th class=modinfopar>Description</th><th class=modinfopar>Default</th><th class=modinfopar>Valid&nbsp;Values</th></tr>\n";
if ($#modparams == -1) {
    print OF "<tr><td colspan=5>This module exposes no parameter</td></tr>\n";
} else {
    foreach my $p (@modparams)
    {
        my %paramHash = %$p;
        print OF "<tr class=modinfopar><td class=modinfopar>$paramHash{name}</td><td class=modinfopar>$paramHash{type}</td><td class=modinfopar>$paramHash{description}</td><td class=modinfopar>$paramHash{defval}</td><td class=modinfopar>$paramHash{vvals}</td></tr>\n";
    }
}
print OF "</table></td></tr>\n";

# reproduce params.cfg if it exists:
my @tmp = split("/", $inputFilename); $tmp[-1] = "params.cfg"; my $paramscfg = join("/", @tmp);
if ( -f $paramscfg )
{
    my $contents = `/bin/cat $paramscfg`;
    print OF "<tr><td><table class=modinfocfg><tr><td class=modinfocfg><b>params.cfg file</b><hr><pre>$contents</pre></td></tr></table></td></tr>\n";
}

# reproduce script.cfg if it exists:
@tmp = split("/", $inputFilename); $tmp[-1] = "script.cfg"; my $scriptcfg = join("/", @tmp);
if ( -f $scriptcfg )
{
    my $contents = `/bin/cat $scriptcfg`;
    print OF "<tr><td><table class=modinfocfg><tr><td class=modinfocfg><b>script.cfg file</b><hr><pre>$contents</pre></td></tr></table></td></tr>\n";
}


# table for all the tag data:
print OF "<tr><td><table class=modinfomisc>\n";

# dump links to module and component docs:
print OF "<tr class=modinfomisc><th class=modinfomisc>Detailed docs:</th><td class=modinfomisc>";
print OF "<A HREF=\"/basedoc/class${className}.html\">$className</A>";
foreach my $c (@subcomps) { print OF ", <A HREF=\"/basedoc/class${c}.html\">$c</A>"; }
print OF "</td></tr>\n";

#print OF "<tr class=modinfomisc><th class=modinfomisc>JeVois Package:</th><td class=modinfomisc>$tagdata{'jevoispkg'}</td></tr>\n";
#print OF "<tr class=modinfomisc><th class=modinfomisc>JeVois Vendor:</th><td class=modinfomisc>$tagdata{'jevoisvend'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Copyright:</th><td class=modinfomisc>$tagdata{'copyright'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>License:</th><td class=modinfomisc>$tagdata{'license'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Distribution:</th><td class=modinfomisc>$tagdata{'distribution'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Restrictions:</th><td class=modinfomisc>$tagdata{'restrictions'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Support URL:</th><td class=modinfomisc>$tagdata{'supporturl'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Other URL:</th><td class=modinfomisc>$tagdata{'otherurl'}</td></tr>\n";
print OF "<tr class=modinfomisc><th class=modinfomisc>Address:</th><td class=modinfomisc>$tagdata{'address'}</td></tr>\n";

# End of modinfomisc table
print OF "</table></td></tr>\n";

# end of master table:
print OF "</table>\n";
if ($fullhtml) { print OF "</body></html>\n"; }
close OF;

# Delete our temporary directory
`/bin/rm -rf $tmpdirname`;

##############################################################################################################
# Levenshtein distance, copied from http://www.merriampark.com/ldperl.htm
sub levenshtein
{
    # $s1 and $s2 are the two strings
    # $len1 and $len2 are their respective lengths
    #
    my ($s1, $s2) = @_;
    my ($len1, $len2) = (length $s1, length $s2);

    # If one of the strings is empty, the distance is the length
    # of the other string
    #
    return $len2 if ($len1 == 0);
    return $len1 if ($len2 == 0);

    my %mat;

    # Init the distance matrix
    #
    # The first row to 0..$len1
    # The first column to 0..$len2
    # The rest to 0
    #
    # The first row and column are initialized so to denote distance
    # from the empty string
    #
    for (my $i = 0; $i <= $len1; ++$i)
    {
        for (my $j = 0; $j <= $len2; ++$j)
        {
            $mat{$i}{$j} = 0;
            $mat{0}{$j} = $j;
        }

        $mat{$i}{0} = $i;
    }

    # Some char-by-char processing is ahead, so prepare
    # array of chars from the strings
    #
    my @ar1 = split(//, $s1);
    my @ar2 = split(//, $s2);

    for (my $i = 1; $i <= $len1; ++$i)
    {
        for (my $j = 1; $j <= $len2; ++$j)
        {
            # Set the cost to 1 iff the ith char of $s1
            # equals the jth of $s2
            # 
            # Denotes a substitution cost. When the char are equal
            # there is no need to substitute, so the cost is 0
            #
            my $cost = ($ar1[$i-1] eq $ar2[$j-1]) ? 0 : 1;

            # Cell $mat{$i}{$j} equals the minimum of:
            #
            # - The cell immediately above plus 1
            # - The cell immediately to the left plus 1
            # - The cell diagonally above and to the left plus the cost
            #
            # We can either insert a new char, delete a char or
            # substitute an existing char (with an associated cost)
            #
            $mat{$i}{$j} = min([$mat{$i-1}{$j} + 1,
                                $mat{$i}{$j-1} + 1,
                                $mat{$i-1}{$j-1} + $cost]);
        }
    }

    # Finally, the Levenshtein distance equals the rightmost bottom cell
    # of the matrix
    #
    # Note that $mat{$x}{$y} denotes the distance between the substrings
    # 1..$x and 1..$y
    #
    return $mat{$len1}{$len2};
}


##############################################################################################################
# minimal element of a list
sub min
{
    my @list = @{$_[0]};
    my $min = $list[0];
    
    foreach my $i (@list) { $min = $i if ($i < $min); }
    
    return $min;
}

##############################################################################################################
sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+|\s+$//g;
	return $string;
}

##############################################################################################################
sub getParams
{
    my $path = shift;
    my $comp = shift;
    
    my $tmpmodname = "$tmpdirname/mod.C";
    my $tmpmodout = "$tmpdirname/mod.E";


    # we need to handle either 5 or 6 args in JEVOIS_DECLARE_PARAMETER. Use trick described here:
    # http://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
    open MOD, ">$tmpmodname" || die "oops";

    print MOD "#define JEVOISDOX_GET_MACRO(_1,_2,_3,_4,_5,_6,NAME,...) NAME\n";
    print MOD "#define JEVOIS_DECLARE_PARAMETER(...) JEVOISDOX_GET_MACRO(__VA_ARGS__, JEVOISDOX6, JEVOISDOX5, FOO4, FOO3, FOO2)(__VA_ARGS__)\n";
    print MOD "#define JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(...) JEVOISDOX_GET_MACRO(__VA_ARGS__, JEVOISDOX6, JEVOISDOX5, FOO4, FOO3, FOO2)(__VA_ARGS__)\n";
    print MOD "#define JEVOISDOX5(na,ty,de,dv,pc) JEVOIS_DECLARE_PARAMDOX #na JVDOX #ty JVDOX #de JVDOX #dv JVDOX \"\" JVDOX #pc\n";
    print MOD "#define JEVOISDOX6(na,ty,de,dv,vv,pc) JEVOIS_DECLARE_PARAMDOX #na JVDOX #ty JVDOX #de JVDOX #dv JVDOX #vv JVDOX #pc\n";
    close MOD;
    
    system("grep -ve \"^#include\" $path | grep -ve \"^#pragma\" >> $tmpmodname");

    system("g++ -std=c++14 -E $tmpmodname | grep JEVOIS_DECLARE_PARAMDOX > $tmpmodout");

    open MODOUT, $tmpmodout || die "oops";
    while (my $line = <MODOUT>)
    {
        # cleanup description
        chomp $line; $line =~ s/\\\" \\\"//g; $line =~ s/\\\"//g;
        
        # split fields and cleanup:
        my @tmp = split(/ JVDOX /, $line);
        $tmp[0] =~ s/\s*JEVOIS_DECLARE_PARAMDOX //;

        for (my $i = 0; $i < 5; $i ++)
        { $tmp[$i] =~ s/^\"//; $tmp[$i] =~ s/\"$//; $tmp[$i] = encode_entities($tmp[$i]); }

        if ($tmp[4] eq "") { $tmp[4] = "-"; }
        
        # store the data
        my $paraminfo = { name => "(<A HREF=\"/basedoc/class${comp}.html\">$comp</A>) $tmp[0]",
                          type => $tmp[1], description => $tmp[2],
                          defval => $tmp[3], vvals => $tmp[4] };
        push @modparams, $paraminfo;
    }
    close MODOUT;
    
    
}
