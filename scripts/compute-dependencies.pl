#!/usr/bin/perl
# Usage: compute_dependencies.pl <exec|so> [...]

use strict;
my %deps;
my @ignored = qw/linux-vdso.so.1 libstdc++.so.6 libgcc_s.so.1 libc.so.6 /;

foreach my $f (@ARGV)
{
    print STDERR "Processing $f ...\n";
    
    # Run ldd to get a list of dependent libs:
    my @ldd = `ldd $f`;

    foreach my $lib (@ldd)
    {
        $lib =~ s/^\s+//;
        ($lib, my $x) = split(/\s+/, $lib, 2);
        
        # Skip if on our ignore list:
        my $skip = 0;
        foreach my $i (@ignored) { if ($i eq $lib) { $skip = 1; last; } }
        next if $skip;

        # Find package(s) for that lib:
        my @packs = `dpkg -S $lib`;
        foreach my $p (@packs)
        {
            (my $deb, my $x) = split(/[: ]/, $p, 2);
            print STDERR "   $deb\n" if (! exists($deps{$deb}));
            $deps{$deb} = 1;
        }
    }
}

# Final output:
foreach my $deb (keys %deps) { print "$deb, "; }
print "\n";
