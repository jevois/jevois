#!/usr/bin/perl
# Create an HTML grid with featured jevois modules, to be pasted on some web page


my $nx = 4; # number of cells horizontally
my $uroot = "http://jevois.org"; # root url
my $iconsize = 80; # icon size

my $mods = [
    [ "DNN", "JeVois-Pro", "deep neural nets" ],
    [ "PyLLM", "JeVois-Pro", "generative AI" ],
    [ "PyHandDetector", "JeVois-Pro mediapipe", "hand detection" ],
    [ "PyFaceMesh", "JeVois-Pro mediapipe", "face mesh detection" ],
    [ "MultiDNN", "JeVois-Pro parallel", "DNN accelerators" ],
    [ "DetectionDNN", "Locate and identify", "faces or objects" ],
    [ "PyDetectionDNN", "Locate and identify", "faces or objects in Python" ],
    [ "PyClassificationDNN", "Recognize 1000 object", "types in Python" ],
    [ "PyEmotion", "Classify facial emotions", "in Python" ],
    [ "TensorFlowSaliency", "Detect and recognize", "1000 object types" ],
    [ "TensorFlowEasy", "Recognize 1000 objects", "at up to 83 fps" ],
    [ "DarknetSaliency", "Detect and recognize", "1000 object types" ],
    [ "DarknetSingle", "Recognize 1000", "objects types" ],
    [ "DarknetYOLO", "Locate and identify", "objects in scenes" ],
    [ "PythonObject6D", "Vision example for", "FIRST Robotics 2018" ],
    [ "AprilTag", "JeVois-Pro April Tag", "detection in Python" ],
    [ "CalibrateCamera", "Camera calibration", "for 3D object pose" ],
    [ "DemoSaliency", "Detect attention-grabbing", "objects" ],
    [ "DemoSalGistFaceObj", "Find faces and", "handwritten digits" ],
    [ "RoadNavigation", "Detect roads", " " ],
    [ "SurpriseRecorder", "Detect and record", "surprising events" ],
    [ "DemoQRcode", "Detect and decode", "QR-codes and barcodes" ],
    [ "DemoARtoolkit", "Detect augmented", "reality markers" ],
    [ "DemoArUco", "Detect and decode", "ArUco tags" ],
    [ "MarkersCombo", "Detect multiple", "kinds of tags" ],
    [ "PyPoseDetector", "JeVois-Pro mediapipe", "body pose detection" ],
    [ "ObjectDetect", "Detect objects", "by keypoint matching" ],
    [ "ObjectTracker", "Track objects", "by color" ],
    [ "OpticalFlow", "Compute motion", "flow at 100 Hz" ],
    [ "SaliencySURF", "Detect and match", "salient landmarks" ],
    [ "DemoBackgroundSubtract", "Find", "moving objects" ],
    [ "DemoEyeTracker", "Track pupil and", "gaze at 120 Hz" ],
    [ "DemoGPU", "Filter video", "using OpenGL-ES 2.0" ],
    [ "ColorFiltering", "Morphological", "processing" ],
    [ "PyLicensePlate", "JeVois-Pro", "license plate detection" ],
    [ "SuperPixelSeg", "Segment video", "into super-pixels" ],
    [ "EdgeDetectionX4", "Detect edges at", "4 scales simultaneously" ],
    [ "SalientRegions", "Grab the 3 most", "salient regions of interest" ],
    [ "DiceCounter", "Count", "dice pips" ],
    [ "DemoNeon", "Fast image", "filtering using NEON" ],
    [ "SaveVideo", "Record video", "to microSD" ],
    [ "DenseSift", "Dense", "SIFT keypoints" ],
    [ "FirstPython", "FIRST Robotics", "object detection" ],
    [ "ArUcoBlob", "ArUco tag +", "color blob tracking" ],
    [ "PythonSandbox", "Your algorithm", "in Python + OpenCV" ],
    [ "DemoIMU", "Optional global shutter", "sensor with IMU" ],
    [ "DemoDMP", "Digital motion processing", "on global shutter sensor IMU" ],
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
