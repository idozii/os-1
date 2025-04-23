RED='\e[31m'
GREEN='\e[32m'
NC='\e[0m'

TC_DIR="_OUTPUT_"
CFGFL="manifest"

IDX=0
SUCCESS=0
ERROR=0

install_manifest() {
    echo "[INSTALL] Downloading manifest from GitHub..."
    wget -O "$CFGFL" https://raw.githubusercontent.com/Trung4n/TestCase4U/main/ossim_sierra/manifest
    if [[ $? -ne 0 ]]; then
        echo -e "${RED}Failed to download manifest.${NC}"
        exit 1
    fi

    echo -e "${GREEN}Manifest downloaded successfully!${NC}"
    echo "[INFO] Downloading files listed in manifest..."

    download_file() {
        local filename="$1"
        local url="https://raw.githubusercontent.com/Trung4n/TestCase4U/main/ossim_sierra/input/$filename"
        local dest="input/$filename"

        wget -O "$dest" "$url"

    }

    export -f download_file
    export GREEN RED NC

    cat "$CFGFL" | xargs -I{} -P 8 bash -c 'download_file "$@"' _ {}

    echo -e "${GREEN}All available files downloaded!${NC}"
    echo "[INFO] Made by Trung4n"
    echo -e "${GREEN}If you find this testcase helpful, please give me a star on GitHub <3 ${NC}"
    exit 0
}

clean_files() {
    if [[ ! -f "$CFGFL" ]]; then
        echo -e "${RED}Manifest file not found: $CFGFL${NC}"
        exit 1
    fi

    echo "[CLEAN] Reading file list from manifest..."

    while IFS= read -r filename || [[ -n $filename ]]; do
        filepath="input/$filename"
        if [[ -f "$filepath" ]]; then
            rm "$filepath"
        fi
    done < "$CFGFL"

    rm $CFGFL
    rm -rf $TC_DIR/*.txt
    rm -r $TC_DIR

    echo -e "${GREEN}Clean completed.${NC}"
    echo "[INFO] Made by Trung4n"
    echo -e "${GREEN}If you find this testcase helpful, please give me a star on GitHub <3 ${NC}"
    exit 0
}

case "$1" in
    run)
        ;;
    install)
        install_manifest
        ;;
    clean)
        clean_files
        ;;
    *)
        echo "Invalid option: $1"
        echo "Usage: ./tc4u <command>"
        echo ""
        echo "Available commands:"
        echo "  install   Download manifest and test input files from GitHub"
        echo "  run       Run the test cases"
        echo "  clean     Clean up files listed in the manifest"
        echo "[INFO] Made by Trung4n"
        exit 1
esac


print_result() {
    if [[ $1 -eq 0 ]]; then
        echo -e "${GREEN}$2${NC}"
        ((SUCCESS++))
    else
        echo -e "${RED}$2${NC}"
        ((ERROR++))
    fi
    ((IDX++))
}

print_summary(){
    echo -e "-----------------------------------------------------------------------"
    echo "Test Summary:"
    echo -e "Success: ${GREEN}$SUCCESS${NC}"
    echo -e "Error: ${RED}$ERROR${NC}"
    echo "Note: This testcase is still under development and may contain some bugs, please report them on Github."
    echo -e "-----------------------------------------------------------------------"
    echo "[INFO] Made by Trung4n"
    echo -e "${GREEN}If you find this testcase helpful, please give me a star on GitHub <3 ${NC}"

    exit 0
}
mkdir -p $TC_DIR
rm -rf $TC_DIR/*.txt

# Start building
echo -e "[BUILDING] Starting make all..."
if ! make all > /dev/null 2>&1; then
    make all
    echo -e "${RED}Oh!, your code has a problem.${NC}"
    exit 1
fi
echo -e "${GREEN}make all succeeded! Build successful!${NC}"
echo -e "[CHECKING] Starting check manifest..."

if [[ ! -f "$CFGFL" ]]; then
    echo -e "${RED}Oh! It looks like you haven't downloaded the manifest from GitHub.${NC}"
    echo -e "Please run ${GREEN}./tc4u install${NC} first."
    exit 1
fi

# Start testing
echo -e "-----------------------------------------------------------------------"
echo -e "Starting OS self-test..."
echo -e "Output files ${RED}(_OUTPUT_/os_an_*.txt)${NC} will be used for result verification."
echo -e "This test case is created by ${GREEN}Trung4n.${NC} "
echo -e "-----------------------------------------------------------------------"

echo "[RUNNING] Testcase 0"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1; then
    print_result 0 "Success"
else
    print_result 1 "Error"
    echo "Please check os-cfg.h and comment out MM_FIXED_MEMSZ"
    print_summary
fi

echo "[RUNNING] Testcase 0.1"
if ./os os_an_01 > "$TC_DIR/os_an_01.txt" 2>&1 && \
    grep -Fxq "PID=1 - Region=1 - Address=00000000 - Size=100 byte" "$TC_DIR/os_an_01.txt" && \
    grep -Fxq "PID=1 - Region=1" "$TC_DIR/os_an_01.txt"; then
    print_result 0 "Success"
else
    print_result 1 "Error"
    echo "Please configure debug output to match the teacher's output file for testing."
    print_summary
fi

((IDX--))

echo "[RUNNING] Testcase 1"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 2"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 3"

if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 4"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "PID=1 - Region=1 - Address=00000000 - Size=1280 byte" "$TC_DIR/os_an_$IDX.txt"; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 5"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "PID=1 - Region=2 - Address=00000200 - Size=212 byte" "$TC_DIR/os_an_$IDX.txt"; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 6"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "PID=1 - Region=1 - Address=00000201 - Size=99 byte" "$TC_DIR/os_an_$IDX.txt"; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 7"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "OOM: vm_map_ram out of memory " "$TC_DIR/os_an_$IDX.txt" && \
   ! grep -Fxq "PID=1 - Region=1 - Address=00000400 - Size=1280 byte" "$TC_DIR/os_an_$IDX.txt" && \
   grep -Fxq "PID=1 - Region=2 - Address=00000400 - Size=1024 byte" "$TC_DIR/os_an_$IDX.txt" ; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 8"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "PID=1 - Region=0 - Address=00000000 - Size=512 byte" "$TC_DIR/os_an_$IDX.txt"; then
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 9"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   (
     grep -Fxq "PID=1 - Region=1 - Address=00000064 - Size=100 byte" "$TC_DIR/os_an_$IDX.txt" || \
     grep -Fxq "PID=1 - Region=1 - Address=00000200 - Size=100 byte" "$TC_DIR/os_an_$IDX.txt" || \
     grep -Fxq "PID=1 - Region=1 - Address=00000338 - Size=100 byte" "$TC_DIR/os_an_$IDX.txt"
   ); then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 10"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "The procname retrieved from memregionid 1 is \"andeptrai\"" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 11"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 12"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   ! grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 13"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 14"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
   ! grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 15"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
     grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt" && \
   ! grep -Fxq "	CPU 0: Processed  2 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 16"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 ; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 17"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
    (
    grep -Fxq "	CPU 0: Processed 13 has finished" "$TC_DIR/os_an_$IDX.txt" || \
    grep -Fxq "	CPU 1: Processed 13 has finished" "$TC_DIR/os_an_$IDX.txt"
    ); then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

# If you fail this case, run again or inbox me
echo "[RUNNING] Testcase 18"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
    (
    grep -Fxq "	CPU 0: Processed 13 has finished" "$TC_DIR/os_an_$IDX.txt" || \
    grep -Fxq "	CPU 1: Processed 13 has finished" "$TC_DIR/os_an_$IDX.txt" || \
    grep -Fxq "	CPU 2: Processed 13 has finished" "$TC_DIR/os_an_$IDX.txt"
    ) && \
    (
    ! grep -Fxq "	CPU 0: Processed 14 has finished" "$TC_DIR/os_an_$IDX.txt" && \
    ! grep -Fxq "	CPU 1: Processed 14 has finished" "$TC_DIR/os_an_$IDX.txt" && \
    ! grep -Fxq "	CPU 2: Processed 14 has finished" "$TC_DIR/os_an_$IDX.txt"
    ); then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

echo "[RUNNING] Testcase 19"
if ./os os_an_$IDX > "$TC_DIR/os_an_$IDX.txt" 2>&1 && \
    grep -Fxq "	CPU 0: Processed  1 has finished" "$TC_DIR/os_an_$IDX.txt"; then 
    print_result 0 "Success"
else
    print_result 1 "Error"
fi

print_summary