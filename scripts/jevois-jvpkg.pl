#!/usr/bin/perl
# USAGE: jevois-jvpkg.pl <dest>
# Assumes that current working directory is in the jvpkg/ folder to archive

use File::Basename;

my @excludes=qw/screenshot*.* icon*.* video*.* modinfo.* *.H *.C *.h *.c *.cpp jvpkg-exclude.cfg/;

my @elist = `find . -name jvpkg-exclude.cfg`;
foreach my $e (@elist) {
    chomp $e; my $dir = dirname($e); #$dir =~s/^\.\///;
    open F, $e || die "oops";
    while (my $line = <F>) { chomp $line; push(@excludes, "$dir/$line"); }
    close F;
}

my $tmpfile = `mktemp`; chomp $tmpfile;
open F, ">$tmpfile" || die "ooops";
foreach my $e (@excludes) { print F "$e\n"; }
close F;

system("tar jcvf $ARGV[0] . --exclude-from $tmpfile");

unlink($tmpfile);

    
