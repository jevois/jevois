#!/usr/bin/perl

# Copyright (C) 2016 by JeVois Inc. -- All rights Reserved.

# USAGE: ./extract-code-snippets.pl will extract the snippets into doc/snip
#
# in your code, use:
# ...
# // BEGIN_JEVOIS_CODE_SNIPPET blackboard-post.C
# // example of post:
# std::unique_ptr<MyMessage> msg(new MyMessage(x, y, z));
# post<MyPort>(msg);
# // END_JEVOIS_CODE_SNIPPET
#

# this will create doc/snip/blackboard-post.C with the code in-between the delimiters

use File::Basename;
use strict;

my $d = dirname($0);

my @flist = `$d/list-sources.sh`;
my $count = $#flist + 1;

print STDERR "extract-code-snippets.pl: Extracting code snippets from $count files in $d ...\n";

foreach my $f (@flist) {
    chomp $f; my $snip = ""; my $sname = "";

    open F, "< $f" || die "Cannot open $f: $!";
    my $count = 1;
    while (my $line = <F>) {
        if ($line =~ m/BEGIN_JEVOIS_CODE_SNIPPET/) {
            if ($sname) { err($f, $count, "BEGIN_JEVOIS_CODE_SNIPPET while already in a snippet!"); }
            else {
                chomp $line;
                my @tmp = split(/BEGIN_JEVOIS_CODE_SNIPPET\s+/, $line); $sname = $tmp[1];
                @tmp = split(/\s+/, $sname); $sname = $tmp[0];
                $snip = "";
            }
        } elsif ($line =~ m/END_JEVOIS_CODE_SNIPPET/) {
            if ($sname eq "") { err($f, $count, "END_JEVOIS_CODE_SNIPPET while not in a snippet!"); }
            my $sn = "$d/../doc/snip/$sname";

            print STDERR "extract-code-snippets.pl: Writing $sn extracted from $f\n";

            open SNIP, "> $sn" || die "Cannot write $sn: $!";
            print SNIP $snip;
            close SNIP;
            $snip = ""; $sname = "";
        } elsif ($sname) {
            $snip .= $line;
        }

        $count ++;
    }
    close F;
}

print STDERR "extract-code-snippets.pl: Done.\n";


######################################################################
sub err {
    print STDERR "$_[0]:$_[1] $_[2]\n";
}
