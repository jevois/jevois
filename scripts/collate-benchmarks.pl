#!/usr/bin/perl

# USAGE: collate-benchmarks.pl file1.html ... fileN.html
# keeps the last entry for each pipe

foreach $arg (@ARGV)
{
    foreach $line (`cat $arg`)
    {
        $line =~ s/\r//g;
        $line2 = $line;
        $line2 =~ s/^<tr><td class=jvpipe>//;
        my ($key, $junk) = split(/ /, $line2, 2);
        $data{$key} = $line;
    }
}

print "<table>\n";
print "<tr><th>Pipeline</th><th>Input</th><th>Output&nbsp;(dequantized)</th><th>PreProc</th><th>Network</th><th>PostProc</th><th>Total</th><th>FPS</th></tr>\n";

my $prev_acc = "NPU";

foreach $k (sort keys(%data))
{
    my ($acc, $x) = split(/:/, $data{$k}, 2);
    if ($acc ne $prev_acc) { print "<tr><td colspan=8></td></tr>\n"; }
    print "$data{$k}";
    $prev_acc = $acc;
}

print "</table>\n";
