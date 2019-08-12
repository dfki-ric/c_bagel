#! /bin/bash
if [ x`which cmake_debug` = "x" ]; then
    echo "command 'cmake_debug' is not known. Did you source the env script?"
    exit 1;
fi

echo "This will remove the build folder in the current directory."
read -p "Do you want to proceed [Y/n]? " answer
if [[ x${answer} = "x" || ( ${answer} = "Y" || ${answer} = "y" ) ]]; then
    pushd . > /dev/null 2>&1
    echo "(1/5) removing existing build dir"
    rm -rf build/

    echo "(2/5) rebuilding with coverage flags"
    mkdir build
    cd build
    cmake_debug -DTEST_COVERAGE=1 >/dev/null 2>&1
    make -j24 >/dev/null 2>&1

    echo "(3/5) running tests"
    ./tests/test_c_bagel >/dev/null 2>&1

    echo "(4/5) determine coverage"
    mkdir test_coverage
    cd test_coverage
    objDir=../CMakeFiles/c_bagel.dir
    if [[ $VERBOSE ]]; then
        gcov -f -o ${objDir}/src/ ${objDir}/src/*.o
        gcov -f -o ${objDir}/src/node_types ${objDir}/src/node_types/*.o
        gcov -f -o ${objDir}/src/merge_types ${objDir}/src/merge_types/*.o
    else
        gcov -o ${objDir}/src/ ${objDir}/src/*.o >/dev/null 2>&1
        gcov -o ${objDir}/src/node_types ${objDir}/src/node_types/*.o >/dev/null 2>&1
        gcov -o ${objDir}/src/merge_types ${objDir}/src/merge_types/*.o >/dev/null 2>&1
    fi        
    covered=`egrep "^\s*[1-9]" *.gcov | wc -l`
    not_covered=`egrep "^\s*#" *.gcov | wc -l`
    total=$((${covered} + ${not_covered}))
    result=$(echo "scale=2; (100 * ${covered}) / ${total};" | bc -q 2>/dev/null)

    echo "(5/5) test coverage:" ${result} "% of" ${total} "lines containing executable code."
    cd ../../
    if [[ x${KEEP_BUILD} == "x" ]]; then
        rm -rf build/
    fi
    popd > /dev/null 2>&1
fi;
