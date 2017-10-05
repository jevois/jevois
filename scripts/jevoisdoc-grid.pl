#!/usr/bin/perl
# Create an HTML grid with featured jevois modules, to be pasted on some web page


my $nx = 4; # number of cells horizontally
my $uroot = "http://jevois.org"; # root url
my $iconsize = 80; # icon size

my $mods = [ [ "DarknetSingle", "Recognize 1000 types", "of objects" ],
             [ "DarknetYOLO", "Locate and identify", "objects in scenes" ],
             [ "DemoSaliency", "Detect attention-grabbing", "objects" ],
             [ "DemoSalGistFaceObj", "Find faces and", "handwritten digits" ],
             [ "RoadNavigation", "Detect roads", " " ],
             [ "SurpriseRecorder", "Detect and record", "surprising events" ],
             [ "DemoQRcode", "Detect and decode", "QR-codes and barcodes" ],
             [ "DemoARtoolkit", "Detect augmented", "reality markers" ],
             [ "DemoArUco", "Detect and decode", "ArUco tags" ],
             [ "MarkersCombo", "Detect multiple", "kinds of tags" ],
             [ "ObjectDetect", "Detect objects", "by keypoint matching" ],
             [ "ObjectTracker", "Track objects", "by color" ],
             [ "OpticalFlow", "Compute", "motion flow" ],
             [ "SaliencySURF", "Detect and match", "salient landmarks" ],
             [ "DemoBackgroundSubtract", "Find", "moving objects" ],
             [ "DemoEyeTracker", "Track pupil", "and gaze direction" ],
             [ "DemoGPU", "Filter video", "using OpenGL-ES 2.0" ],
             [ "ColorFiltering", "Morphological", "processing" ],
             [ "SuperPixelSeg", "Segment video", "into super-pixels" ],
             [ "EdgeDetectionX4", "Detect edges at", "4 scales simultaneously" ],
             [ "SalientRegions", "Grab the 3 most", "salient regions of interest" ],
             [ "DiceCounter", "Count", "dice pips" ],
             [ "DemoNeon", "Fast image", "filtering using NEON" ],
             [ "SaveVideo", "Record video", "to microSD" ],
             [ "DenseSift", "Dense", "SIFT keypoints" ],
             [ "PythonSandbox", "Your algorithm", "in Python + OpenCV" ],
    ];

        
print "<table width=100% cellspacing=30>\n";
my $n = 0;
     

for my $m (@$mods) {
    my ($name, $desc1, $desc2) = @{$m}[0,1,2];

    if ($n == 0) { print "<tr>\n"; }

    my $iurl = "${uroot}/moddoc/$name/icon.png";
    my $murl = "${uroot}/moddoc/$name/modinfo.html";
    
    print "<td align=center><table><tr><td align=center><a href=\"$murl\"><img src=\"$iurl\" width=$iconsize></td></tr>";
    print "<tr><td align=center><a href=\"$murl\">$desc1</a></td></tr>";
    print "<tr><td align=center><a href=\"$murl\">$desc2</a></td></tr>";
    print "</table></td>\n";
    
    $n ++;
    if ($n == $nx) { print "</tr><tr><td colspan=$nx>&nbsp;</td></tr><tr><td colspan=$nx>&nbsp;</td></tr>\n"; $n = 0; }
}
print "</table>\n";
