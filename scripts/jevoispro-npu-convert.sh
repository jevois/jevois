#!/bin/bash

# Simple script to convert a model for NPU. Needs to be run in the directory where './convert' is in the aml-npu-sdk.
# 
# USAGE: jevoispro-npu-convert.sh <base><version><size>[-task]-<suffix>.onnx [dataset.txt] [iterations]
#
# Where:
#
# - base: yolo (will add more later)
# - version: v8, v9, v10, 11
# - size: n (nano, fastest but less accurate), t (tiny), s (small), m (medium), etc
# - task: none or -det for detection
#         -seg (detection + segmentic intance segmentation)
#         -pose (detection + pose/skeleton estimation)
#         -obb (detection with oriented bounding boxes)
#         -cls (classification)
#         -worldv2 or -jevois (open-vocabulary any object detection with yolo-world)
# - suffix: something that makes your model name different than the existing ones, e.g.,
#     yolov8n-1024x576.onnx, yolo11s-pose-custom.onnx
# - dataset.txt: a text file containing a list of images to use for quantization. These should be part of the validation
#     set used to train the model. Typically 1000 images or so. If no dataset file is given, will try to use
#     dataset-<task>.txt in the current directory.
# - iterations: number of images from dataset.txt to use for quantization, or use all if not given.

set -e # abort on any error

verbose=1  # Set to 0 for quiet conversion, or 1 for verbose

onnx=$1
dataset=$2
iterations=$3

# A few minimal checks of user input:
if [[ ! -x "convert" ]]; then echo "Must run inside aml_npu_sdk/acuity-toolkit/python/ where ./convert is"; exit 1; fi
if [[ ! -f "$onnx" ]]; then echo "Cannot find ONNX model file $onnx -- ABORT"; exit 3; fi

# Extract model name, name without suffix, and task:
name=${onnx%\.onnx}
model=${name%%-*}
if [[ $model == $name ]]; then echo "You need to provide a suffix to differentiate your model -- ABORT"; exit 4; fi

taskandsuffix=${name#*-}  # take everything after first dash
task=${taskandsuffix%%-*} # now only keep until first dash

# If no task given, assume detection task:
if [[ $task != det && $task != cls && $task != seg && $task != pose && $task != obb && $task != worldv2 ]]; then
    task="det";
fi

# If no dataset name given use dataset-<task>.txt, then check that dataset exists:
if [[ $dataset == "" ]]; then dataset="dataset-${task}.txt"; fi
if [[ ! -f "$dataset" ]]; then echo "Cannot find dataset file $dataset -- ABORT"; exit 2; fi

# Use all images if iterations was not given:
if [[ $iterations == "" ]]; then iterations=`cat $dataset | wc -l`; fi

echo "JEVOIS: Convert model=${name} task=${task} dataset=${dataset} iterations=${iterations}"

# Prepare our conversion:
means="0 0 0 0.003921568393707275"
quant="asymmetric_affine"

stdargs=("--model-name" "${name}" "--platform" "onnx" "--model" "${onnx}" "--mean-values" "${means}"
         "--quantized-dtype" "${quant}" "--source-files" "${dataset}" "--kboard" "VIM3" "--print-level" "${verbose}"
         "--batch-size" "1" "--iterations" "${iterations}")

# We just now need to name the output tensors, which is model-dependent:
outs="bad"

case $model in
    yolo*) # ----------------------------------------------------------------------------------------------------
        if [[ $model =~ yolov8 || $model =~ yolov9 ]]; then mv='model.22'; else mv='model.23'; fi
        if [[ $model =~ yolov10 ]]; then oto="one2one_"; else oto=""; fi
        
        case $task in
            cls)
                outs="" # only one output, will be autodetected by converter
                postproc="postproc: Classify"
                ;;
            
            det)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv3.0/${oto}cv3.0.2/Conv
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv3.1/${oto}cv3.1.2/Conv
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv3.2/${oto}cv3.2.2/Conv)
                postproc="postproc: Detect"$'\n'"  detecttype: YOLOv8"
                ;;
            
            seg)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv3.0/${oto}cv3.0.2/Conv
                      /${mv}/${oto}cv4.0/${oto}cv4.0.2/Conv
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv3.1/${oto}cv3.1.2/Conv
                      /${mv}/${oto}cv4.1/${oto}cv4.1.2/Conv
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv3.2/${oto}cv3.2.2/Conv
                      /${mv}/${oto}cv4.2/${oto}cv4.2.2/Conv
                      output1)  # mask prototypes
                postproc="postproc: Detect"$'\n'"  detecttype: YOLOv8seg"
                ;;

            pose)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv3.0/${oto}cv3.0.2/Conv
                      /${mv}/${oto}cv4.0/${oto}cv4.0.2/Conv
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv3.1/${oto}cv3.1.2/Conv
                      /${mv}/${oto}cv4.1/${oto}cv4.1.2/Conv
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv3.2/${oto}cv3.2.2/Conv
                      /${mv}/${oto}cv4.2/${oto}cv4.2.2/Conv)
                postproc="postproc: Pose"$'\n'"  posetype: YOLOv8"
                ;;
            
            obb)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv3.0/${oto}cv3.0.2/Conv
                      /${mv}/${oto}cv4.0/${oto}cv4.0.2/Conv
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv3.1/${oto}cv3.1.2/Conv
                      /${mv}/${oto}cv4.1/${oto}cv4.1.2/Conv
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv3.2/${oto}cv3.2.2/Conv
                      /${mv}/${oto}cv4.2/${oto}cv4.2.2/Conv)
                postproc="postproc: DetectOBB"$'\n'"  detecttypeobb: YOLOv8"
                ;;
            
            worldv2)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv4.0/Add
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv4.1/Add
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv4.2/Add)
                postproc="postproc: Detect"$'\n'"  detecttype: YOLOv8"
                ;;

            jevois)
                outs=(/${mv}/${oto}cv2.0/${oto}cv2.0.2/Conv /${mv}/${oto}cv4.0/Add
                      /${mv}/${oto}cv2.1/${oto}cv2.1.2/Conv /${mv}/${oto}cv4.1/Add
                      /${mv}/${oto}cv2.2/${oto}cv2.2.2/Conv /${mv}/${oto}cv4.2/Add)
                postproc="postproc: Detect"$'\n'"  detecttype: YOLOv8jevois"
                ;;

        esac    
esac

# Stop here if none of the above cases was a match to the user input:
if [[ $outs == bad ]]; then echo "Cannot (yet) convert your model, check your arguments -- ABORT"; exit 30; fi

# echo "${model}-${task}: ${outs[@]}"

# Add _output_0 suffix to all output tensors except those named "output*", as the NPU converter wants the output names,
# not operation names:
if [[ $outs != "" ]]; then
    oparam=""
    spc=""
    for o in "${outs[@]}"; do
        if [[ ! "$o" =~ output* ]]; then oparam="${oparam}${spc}${o}_output_0"; else oparam="${oparam}${spc}${o}"; fi
        spc=' '
    done
    args=("${stdargs[@]}" "--outputs" "'${oparam}'")
else
    args=${stdargs}
fi

# Run the conversion
echo "JEVOIS RUNNING: ./convert ${args[@]}"
echo
./convert "${args[@]}" #--device GPU #2>&1 | sed -u '/^$/d' # sed is to remove empty lines generated by convert program

if [[ ! -d "outputs/${name}" ]]; then
    echo "JEVOIS: model conversion script failed to produce output."
    echo "Try converting your model using the 0_import_model.sh, etc in acuity-toolkit/demo/ -- ABORT"
    exit 23
fi

# Create the yaml file:
cat > "outputs/${name}/${name}.yml" <<EOF
%YAML 1.0
---

${name}:
  preproc: Blob
  mean: "0 0 0"
  scale: 0.003921568393707275
  nettype: NPU
  model: "dnn/custom/${name}.nb"
  library: "dnn/custom/libnn_${name}.so"
  comment: "Converted using JeVois NPU SDK and Asymmetric Affine quant"
  ${postproc}
  # Create a text file with your own class names:
  #classes: "dnn/labels/coco-labels.txt"
EOF

echo "JEVOIS SUCCESS: compiled NPU model, library, and .yml config are in outputs/${name}/"
echo "Copy them to JEVOISPRO:/share/dnn/custom/ on microSD to use the model."
