NAME="test_des_perf_1"
PATH="/home/seamus/repo/research/"$NAME
PLACER="../bin/placer"

$PLACER -flow iccad2017 \
-onlyMGL \
-refinement \
-IgnoreFence \
-DisablePinAcc \
-DisableWeightedDisp \
-cell_lef $PATH/cells_modified.lef \
-tech_lef $PATH/tech.lef \
-input_def $PATH/placed.def \
-output_def $PATH/$NAME".def" \
-placement_constraints $PATH/placement.constraints  | tee -a logs.txt