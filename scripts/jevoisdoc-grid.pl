#!/usr/bin/perl
# Create an HTML grid with featured jevois modules, to be pasted on some web page


my $nx = 4; # number of cells horizontally
my $uroot = "http://jevois.org"; # root url
my $iconsize = 80; # icon size

my $mods = [ [ "DarknetSingle", "Recognize 1000 types of&nbsp;objects" ],
             [ "DarknetYOLO", "Locate and identify objects&nbsp;in&nbsp;scenes" ],
             [ "DemoSaliency", "Detect attention-grabbing objects" ],
             [ "DemoSalGistFaceObj", "Find faces and handwritten&nbsp;digits" ],
             [ "RoadNavigation", "Detect roads" ],
             [ "SurpriseRecorder", "Detect and record surprising&nbsp;events" ],
             [ "DemoQRcode", "Detect and decode QR-codes and&nbsp;barcodes" ],
             [ "DemoARtoolkit", "Detect augmented reality&nbsp;markers" ],
             [ "DemoArUco", "Detect and decode ArUco&nbsp;tags" ],
             [ "MarkersCombo", "Detect multiple kinds&nbsp;of&nbsp;tags" ],
             [ "ObjectDetect", "Detect objects by keypoint&nbsp;matching" ],
             [ "ObjectTracker", "Track objects by&nbsp;color" ],
             [ "OpticalFlow", "Compute motion flow" ],
             [ "SaliencySURF", "Detect and match salient&nbsp;landmarks" ],
             [ "DemoBackgroundSubtract", "Find moving objects" ],
             [ "DemoEyeTracker", "Track pupil and gaze&nbsp;direction" ],
             [ "DemoGPU", "Filter video using OpenGL-ES&nbsp;2.0" ],
             [ "ColorFiltering", "Morphological processing" ],
             [ "SuperPixelSeg", "Segment video into super-pixels" ],
             [ "EdgeDetectionX4", "Detect edges at 4 scales&nbsp;simultaneously" ],
             [ "SalientRegions", "Grab the 3 most salieny regions&nbsp;of&nbsp;interest" ],
             [ "DiceCounter", "Count dice pips" ],
             [ "DemoNeon", "Fast image filtering using&nbsp;NEON" ],
             [ "SaveVideo", "Record video to&nbsp;microSD" ],
             [ "DenseSift", "Dense SIFT keypoints" ],
             [ "PythonSandbox", "Your algorithm in Python&nbsp;+&nbsp;OpenCV" ],
    ];

        
print "<table width=100% cellpadding=30>\n";
my $n = 0;
     

for my $m (@$mods) {
    my ($name, $desc) = @{$m}[0,1];

    if ($n == 0) { print "<tr>\n"; }

    my $iurl = "${uroot}/moddoc/$name/icon.png";
    my $murl = "${uroot}/moddoc/$name/modinfo.html";
    
    print "<td><table><tr><td align=center><a href=\"$murl\"><img src=\"$iurl\" width=$iconsize></td></tr>";
    print "<tr><td align=center><a href=\"$murl\">$desc</a></td></tr></table></td>\n";
    
    $n ++;
    if ($n == $nx) { print "</tr><tr><td colspan=$nx>&nbsp;</td></tr><tr><td colspan=$nx>&nbsp;</td></tr>\n"; $n = 0; }
}
print "</table>\n";
