#!/bin/bash
# @file: run.sh
# @author: George Ungureanu, KTH/EECS/ELE
# @date: 21.08.2018
# @version: 0.1
#
# This is a bash script for automating the compilation and deployment
# of the Nios II project in the current folder. It is a more readable
# (albeit less powerful) version of the 'Makefile' one folder
# above. It is recommended for beginner students to understand what is
# happening during the compilation process.

# Paths for DE2-35 sources
CORE_FILE=/home/student/Documents/IL2206/hardware/nios2_ht18_Eriksson_Keyserling/nios2_ht18_Eriksson_keyerlingk.sopcinfo
SOF_FILE=/home/student/Documents/IL2206/hardware/nios2_ht18_Eriksson_Keyserlingk/output_files/nios2_ht18_Eriksson_keyserlingk.sof
JDI_FILE=/home/student/Documents/IL2206/hardware/nios2_ht18_Eriksson_Keyserling/output_files/nios2_ht18_Eriksson_keyserlingk.jdi

# Paths for DE2-115 sources
# CORE_FILE=../../hardware/DE2-115-pre-built/DE2_115_Nios2System.sopcinfo
# SOF_FILE=../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.sof
# JDI_FILE=../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.jdi

APP_NAME=lab1-io
CPU_NAME=nios2_ht18_Eriksson_keyserlignk
BSP_PATH=/home/student/Documents/IL2206/hardware/nios2_ht18_Eriksson_Keyserlingk/software/nios2_ht18_Eriksson_keyserlingk_bsp
SRC_PATH=./src

echo -e "\n******************************************"
echo -e   "Building the BSP and compiling the program"
echo -e   "******************************************\n"

nios2-bsp hal $BSP_PATH $CORE_FILE \
	  --cpu-name $CPU_NAME \
	  --set hal.make.bsp_cflags_debug -g \
	  --set hal.make.bsp_cflags_optimization -O0 \
	  --set hal.enable_sopc_sysid_check 1 \
	  --set hal.max_file_descriptors 4

nios2-app-generate-makefile \
    --bsp-dir $BSP_PATH \
    --elf-name $APP_NAME.elf \
    --src-dir $SRC_PATH \
    --set APP_CFLAGS_OPTIMIZATION -O0

make 3>&1 1>>log.txt 2>&1

echo -e "\n**************************"
echo -e  "Download hardware to board"
echo -e  "**************************\n"

nios2-configure-sof $SOF_FILE


echo -e "\n**************************"
echo -e   "Download software to board"
echo -e   "**************************\n"

xterm -e "nios2-terminal -i 0" &
nios2-download -g $APP_NAME.elf --cpu_name $CPU_NAME --jdi $JDI_FILE


echo ""
echo "Code compilation errors are logged in 'log.txt'"
