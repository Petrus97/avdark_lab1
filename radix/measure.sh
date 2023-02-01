#!/bin/bash

AVDC_ROOT=../avdark-cache
AVDC_SCRIPT=${AVDC_ROOT}/pin-avdc.sh

OUT_DIR=measurements

BINARY=radix
CMD=(./${BINARY} -n 100000)

mkdir -p ${OUT_DIR}

SIZES=($(for i in {14..16} ; do echo $((2**$i)) ; done))
BLOCKS=($(for i in {4..6} ; do echo $((2**$i)) ; done))
ASSOCS=(1 2)

out_table=${OUT_DIR}/${BINARY}-missratio.csv
echo "Cache size,Line size,Associativity,Miss Ratio" | tee ${out_table}
for a in ${ASSOCS[@]} ; do
    for b in ${BLOCKS[@]} ; do
        for s in ${SIZES[@]} ; do
            out_file=${OUT_DIR}/${BINARY}-s${s}-l${b}-a${a}.out
            ${BASH} ${AVDC_SCRIPT} -o ${out_file} -s ${s} -l ${b} -a ${a} -- ${CMD[@]} > /dev/null
            echo -n "${s},${b},${a}," | tee -a ${out_table}
            cat ${out_file} | grep 'Miss Ratio' | cut -d ':' -f 2 | tr -d ' ' | tee -a ${out_table}
        done
    done
done
