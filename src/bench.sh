#!/bin/bash
PLACER="../bin/placer"
ALLTESTCASE="mgc_perf_a"
# ALLTESTCASE="mgc_perf_b mgc_fft_1 mgc_fft_a mgc_perf_a edit_dist mgc_fft_b"

# fun(){
#   for CASE in $ALLTESTCASE ;do
#     #for i in {1..5} ;do
#         echo $CASE
#         # DATA_PATH="/home/tshung/jdp_project/testcase/double_the_row/"$CASE"/double_the_coordinate"
#         DATA_PATH="/home/tshung/jdp_project/testcase/double_the_row/"$CASE
#         IN_TECH_NAME="tech.lef"
#         IN_LEF_NAME="FULL_PPNN.lef"
#         IN_DEF_NAME="ver20/legalized_mode1_addSite.def"
#         OUT_DEF_NAME="BEST_Ver20.def"
#         mkdir -p $DATA_PATH"/ripple_legalized/"

#         $PLACER -flow iccad2017 \
#         -IgnoreFence \
#         -DisablePinAcc \
#         -DisableWeightedDisp \
#         -cell_lef $DATA_PATH"/"$IN_LEF_NAME \
#         -tech_lef $DATA_PATH/$IN_TECH_NAME \
#         -input_def $DATA_PATH"/"$IN_DEF_NAME \
#         -output_def $DATA_PATH"/ripple_legalized/"$OUT_DEF_NAME
#     #done
#   done
# }
# fun

fun(){
  for CASE in $ALLTESTCASE ;do
        echo $CASE
        DATA_PATH3="/home/tshung/jdp_project/testcase/double_the_row/"$CASE"/double_the_coordinate"
        # DATA_PATH4="/home/tshung/jdp_project/testcase/double_the_row/"$CASE
        # IN_LEF_NAME="FULL.lef"
        IN_LEF_NAME="FULL_PPNN.lef"
        IN_DEF_NAME="BEST_Ver20_innovusOut.def"
        OUT_DEF_NAME="BEST_Ver20_addSite.def"
        mkdir -p $DATA_PATH3"/ripple_legalized/" 

        # modified Ripple, only do MGL legalize
        $PLACER -flow iccad2017 \
        -IgnoreFence \
        -DisablePinAcc \
        -DisableWeightedDisp \
        -onlyMGL \
        -cell_lef $DATA_PATH3/$IN_LEF_NAME \
        -input_def $DATA_PATH3/$IN_DEF_NAME \
        -output_def $DATA_PATH3"/ripple_legalized/"$OUT_DEF_NAME
  done
}
fun