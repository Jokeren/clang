///
/// Perform several driver tests for OpenMP offloading
///

// REQUIRES: clang-driver
// REQUIRES: x86-registered-target
// REQUIRES: powerpc-registered-target
// REQUIRES: nvptx-registered-target

/// ###########################################################################

/// Check whether an invalid OpenMP target is specified:
// RUN:   %clang -### -fopenmp=libomp -fopenmp-targets=aaa-bbb-ccc-ddd %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-INVALID-TARGET %s
// CHK-INVALID-TARGET: error: OpenMP target is invalid: 'aaa-bbb-ccc-ddd'

/// ###########################################################################

/// Check warning for empty -fopenmp-targets
// RUN:   %clang -### -fopenmp=libomp -fopenmp-targets=  %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-EMPTY-OMPTARGETS %s
// CHK-EMPTY-OMPTARGETS: warning: joined argument expects additional value: '-fopenmp-targets='

/// ###########################################################################

/// Check error for no -fopenmp option
// RUN:   %clang -### -fopenmp-targets=powerpc64le-ibm-linux-gnu  %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-NO-FOPENMP %s
// RUN:   %clang -### -fopenmp=libgomp -fopenmp-targets=powerpc64le-ibm-linux-gnu  %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-NO-FOPENMP %s
// CHK-NO-FOPENMP: error: The option -fopenmp-targets must be used in conjunction with a -fopenmp option compatible with offloading, please use -fopenmp=libomp or -fopenmp=libiomp5.

/// ###########################################################################

/// Check warning for duplicate offloading targets.
// RUN:   %clang -### -ccc-print-phases -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu,powerpc64le-ibm-linux-gnu  %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-DUPLICATES %s
// CHK-DUPLICATES: warning: The OpenMP offloading target 'powerpc64le-ibm-linux-gnu' is similar to target 'powerpc64le-ibm-linux-gnu' already specified - will be ignored.

/// ###########################################################################

/// Check -Xopenmp-target=powerpc64le-ibm-linux-gnu -mcpu=pwr7 is passed when compiling for the device.
// RUN:   %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu -Xopenmp-target=powerpc64le-ibm-linux-gnu -mcpu=pwr7 %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-FOPENMP-EQ-TARGET %s

// CHK-FOPENMP-EQ-TARGET: clang{{.*}} "-target-cpu" "pwr7" {{.*}}"-fopenmp-is-device"

/// ###########################################################################

/// Check -Xopenmp-target -mcpu=pwr7 is passed when compiling for the device.
// RUN:   %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu -Xopenmp-target -mcpu=pwr7 %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-FOPENMP-TARGET %s

// CHK-FOPENMP-TARGET: clang{{.*}} "-target-cpu" "pwr7" {{.*}}"-fopenmp-is-device"

/// ##########################################################################

/// Check -mcpu=pwr7 is passed to the same triple.
// RUN:    %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu -target powerpc64le-ibm-linux-gnu -mcpu=pwr7 %s 2>&1 \
// RUN:    | FileCheck -check-prefix=CHK-FOPENMP-MCPU-TO-SAME-TRIPLE %s

// CHK-FOPENMP-MCPU-TO-SAME-TRIPLE: clang{{.*}} "-target-cpu" "pwr7" {{.*}}"-fopenmp-is-device"

/// ##########################################################################

/// Check -march=pwr7 is NOT passed to nvptx64-nvidia-cuda.
// RUN:    %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=nvptx64-nvidia-cuda -target powerpc64le-ibm-linux-gnu -march=pwr7 %s 2>&1 \
// RUN:    | FileCheck -check-prefix=CHK-FOPENMP-MARCH-TO-GPU %s

// CHK-FOPENMP-MARCH-TO-GPU-NOT: clang{{.*}} "-target-cpu" "pwr7" {{.*}}"-fopenmp-is-device"

/// ###########################################################################

/// Check -march=pwr7 is NOT passed to x86_64-unknown-linux-gnu.
// RUN:    %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=x86_64-unknown-linux-gnu -target powerpc64le-ibm-linux-gnu -march=pwr7 %s 2>&1 \
// RUN:    | FileCheck -check-prefix=CHK-FOPENMP-MARCH-TO-X86 %s

// CHK-FOPENMP-MARCH-TO-X86-NOT: clang{{.*}} "-target-cpu" "pwr7" {{.*}}"-fopenmp-is-device"

/// ###########################################################################

/// Check -Xopenmp-target triggers error when multiple triples are used.
// RUN:   %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu,powerpc64le-unknown-linux-gnu -Xopenmp-target -mcpu=pwr8 %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-FOPENMP-TARGET-AMBIGUOUS-ERROR %s

// CHK-FOPENMP-TARGET-AMBIGUOUS-ERROR: clang{{.*}} error: cannot deduce implicit triple value for -Xopenmp-target, specify triple using -Xopenmp-target=<triple>

/// ###########################################################################

/// Check -Xopenmp-target triggers error when an option requiring arguments is passed to it.
// RUN:   %clang -### -no-canonical-prefixes -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu -Xopenmp-target -Xopenmp-target -mcpu=pwr8 %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-FOPENMP-TARGET-NESTED-ERROR %s

// CHK-FOPENMP-TARGET-NESTED-ERROR: clang{{.*}} error: invalid -Xopenmp-target argument: '-Xopenmp-target -Xopenmp-target', options requiring arguments are unsupported

/// ###########################################################################

/// Check the phases graph when using a single target, different from the host.
/// We should have an offload action joining the host compile and device
/// preprocessor and another one joining the device linking outputs to the host
/// action.
// RUN:   %clang -ccc-print-phases -fopenmp=libomp -target powerpc64le-ibm-linux-gnu -fopenmp-targets=x86_64-pc-linux-gnu %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-PHASES %s
// CHK-PHASES: 0: input, "[[INPUT:.+\.c]]", c, (host-openmp)
// CHK-PHASES: 1: preprocessor, {0}, cpp-output, (host-openmp)
// CHK-PHASES: 2: compiler, {1}, ir, (host-openmp)
// CHK-PHASES: 3: backend, {2}, assembler, (host-openmp)
// CHK-PHASES: 4: assembler, {3}, object, (host-openmp)
// CHK-PHASES: 5: linker, {4}, image, (host-openmp)
// CHK-PHASES: 6: input, "[[INPUT]]", c, (device-openmp)
// CHK-PHASES: 7: preprocessor, {6}, cpp-output, (device-openmp)
// CHK-PHASES: 8: compiler, {7}, ir, (device-openmp)
// CHK-PHASES: 9: offload, "host-openmp (powerpc64le-ibm-linux-gnu)" {2}, "device-openmp (x86_64-pc-linux-gnu)" {8}, ir
// CHK-PHASES: 10: backend, {9}, assembler, (device-openmp)
// CHK-PHASES: 11: assembler, {10}, object, (device-openmp)
// CHK-PHASES: 12: linker, {11}, image, (device-openmp)
// CHK-PHASES: 13: offload, "host-openmp (powerpc64le-ibm-linux-gnu)" {5}, "device-openmp (x86_64-pc-linux-gnu)" {12}, image

/// ###########################################################################

/// Check the phases when using multiple targets. Here we also add a library to
/// make sure it is treated as input by the device.
// RUN:   %clang -ccc-print-phases -lsomelib -fopenmp=libomp -target powerpc64-ibm-linux-gnu -fopenmp-targets=x86_64-pc-linux-gnu,powerpc64-ibm-linux-gnu %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-PHASES-LIB %s
// CHK-PHASES-LIB: 0: input, "somelib", object, (host-openmp)
// CHK-PHASES-LIB: 1: input, "[[INPUT:.+\.c]]", c, (host-openmp)
// CHK-PHASES-LIB: 2: preprocessor, {1}, cpp-output, (host-openmp)
// CHK-PHASES-LIB: 3: compiler, {2}, ir, (host-openmp)
// CHK-PHASES-LIB: 4: backend, {3}, assembler, (host-openmp)
// CHK-PHASES-LIB: 5: assembler, {4}, object, (host-openmp)
// CHK-PHASES-LIB: 6: linker, {0, 5}, image, (host-openmp)
// CHK-PHASES-LIB: 7: input, "somelib", object, (device-openmp)
// CHK-PHASES-LIB: 8: input, "[[INPUT]]", c, (device-openmp)
// CHK-PHASES-LIB: 9: preprocessor, {8}, cpp-output, (device-openmp)
// CHK-PHASES-LIB: 10: compiler, {9}, ir, (device-openmp)
// CHK-PHASES-LIB: 11: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {3}, "device-openmp (x86_64-pc-linux-gnu)" {10}, ir
// CHK-PHASES-LIB: 12: backend, {11}, assembler, (device-openmp)
// CHK-PHASES-LIB: 13: assembler, {12}, object, (device-openmp)
// CHK-PHASES-LIB: 14: linker, {7, 13}, image, (device-openmp)
// CHK-PHASES-LIB: 15: input, "somelib", object, (device-openmp)
// CHK-PHASES-LIB: 16: input, "[[INPUT]]", c, (device-openmp)
// CHK-PHASES-LIB: 17: preprocessor, {16}, cpp-output, (device-openmp)
// CHK-PHASES-LIB: 18: compiler, {17}, ir, (device-openmp)
// CHK-PHASES-LIB: 19: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {3}, "device-openmp (powerpc64-ibm-linux-gnu)" {18}, ir
// CHK-PHASES-LIB: 20: backend, {19}, assembler, (device-openmp)
// CHK-PHASES-LIB: 21: assembler, {20}, object, (device-openmp)
// CHK-PHASES-LIB: 22: linker, {15, 21}, image, (device-openmp)
// CHK-PHASES-LIB: 23: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {6}, "device-openmp (x86_64-pc-linux-gnu)" {14}, "device-openmp (powerpc64-ibm-linux-gnu)" {22}, image


/// ###########################################################################

/// Check the phases when using multiple targets and multiple source files
// RUN:   echo " " > %t.c
// RUN:   %clang -ccc-print-phases -lsomelib -fopenmp=libomp -target powerpc64-ibm-linux-gnu -fopenmp-targets=x86_64-pc-linux-gnu,powerpc64-ibm-linux-gnu %s %t.c 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-PHASES-FILES %s
// CHK-PHASES-FILES: 0: input, "somelib", object, (host-openmp)
// CHK-PHASES-FILES: 1: input, "[[INPUT1:.+\.c]]", c, (host-openmp)
// CHK-PHASES-FILES: 2: preprocessor, {1}, cpp-output, (host-openmp)
// CHK-PHASES-FILES: 3: compiler, {2}, ir, (host-openmp)
// CHK-PHASES-FILES: 4: backend, {3}, assembler, (host-openmp)
// CHK-PHASES-FILES: 5: assembler, {4}, object, (host-openmp)
// CHK-PHASES-FILES: 6: input, "[[INPUT2:.+\.c]]", c, (host-openmp)
// CHK-PHASES-FILES: 7: preprocessor, {6}, cpp-output, (host-openmp)
// CHK-PHASES-FILES: 8: compiler, {7}, ir, (host-openmp)
// CHK-PHASES-FILES: 9: backend, {8}, assembler, (host-openmp)
// CHK-PHASES-FILES: 10: assembler, {9}, object, (host-openmp)
// CHK-PHASES-FILES: 11: linker, {0, 5, 10}, image, (host-openmp)
// CHK-PHASES-FILES: 12: input, "somelib", object, (device-openmp)
// CHK-PHASES-FILES: 13: input, "[[INPUT1]]", c, (device-openmp)
// CHK-PHASES-FILES: 14: preprocessor, {13}, cpp-output, (device-openmp)
// CHK-PHASES-FILES: 15: compiler, {14}, ir, (device-openmp)
// CHK-PHASES-FILES: 16: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {3}, "device-openmp (x86_64-pc-linux-gnu)" {15}, ir
// CHK-PHASES-FILES: 17: backend, {16}, assembler, (device-openmp)
// CHK-PHASES-FILES: 18: assembler, {17}, object, (device-openmp)
// CHK-PHASES-FILES: 19: input, "[[INPUT2]]", c, (device-openmp)
// CHK-PHASES-FILES: 20: preprocessor, {19}, cpp-output, (device-openmp)
// CHK-PHASES-FILES: 21: compiler, {20}, ir, (device-openmp)
// CHK-PHASES-FILES: 22: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {8}, "device-openmp (x86_64-pc-linux-gnu)" {21}, ir
// CHK-PHASES-FILES: 23: backend, {22}, assembler, (device-openmp)
// CHK-PHASES-FILES: 24: assembler, {23}, object, (device-openmp)
// CHK-PHASES-FILES: 25: linker, {12, 18, 24}, image, (device-openmp)
// CHK-PHASES-FILES: 26: input, "somelib", object, (device-openmp)
// CHK-PHASES-FILES: 27: input, "[[INPUT1]]", c, (device-openmp)
// CHK-PHASES-FILES: 28: preprocessor, {27}, cpp-output, (device-openmp)
// CHK-PHASES-FILES: 29: compiler, {28}, ir, (device-openmp)
// CHK-PHASES-FILES: 30: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {3}, "device-openmp (powerpc64-ibm-linux-gnu)" {29}, ir
// CHK-PHASES-FILES: 31: backend, {30}, assembler, (device-openmp)
// CHK-PHASES-FILES: 32: assembler, {31}, object, (device-openmp)
// CHK-PHASES-FILES: 33: input, "[[INPUT2]]", c, (device-openmp)
// CHK-PHASES-FILES: 34: preprocessor, {33}, cpp-output, (device-openmp)
// CHK-PHASES-FILES: 35: compiler, {34}, ir, (device-openmp)
// CHK-PHASES-FILES: 36: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {8}, "device-openmp (powerpc64-ibm-linux-gnu)" {35}, ir
// CHK-PHASES-FILES: 37: backend, {36}, assembler, (device-openmp)
// CHK-PHASES-FILES: 38: assembler, {37}, object, (device-openmp)
// CHK-PHASES-FILES: 39: linker, {26, 32, 38}, image, (device-openmp)
// CHK-PHASES-FILES: 40: offload, "host-openmp (powerpc64-ibm-linux-gnu)" {11}, "device-openmp (x86_64-pc-linux-gnu)" {25}, "device-openmp (powerpc64-ibm-linux-gnu)" {39}, image

/// ###########################################################################

/// Check the phases graph when using a single GPU target, and check the OpenMP
/// and CUDA phases are articulated correctly.
// RUN:   %clang -ccc-print-phases -fopenmp=libomp -target powerpc64le-ibm-linux-gnu -fopenmp-targets=nvptx64-nvidia-cuda -x cuda %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-PHASES-WITH-CUDA %s
// CHK-PHASES-WITH-CUDA: 0: input, "[[INPUT:.+\.c]]", cuda, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 1: preprocessor, {0}, cuda-cpp-output, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 2: compiler, {1}, ir, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 3: input, "[[INPUT]]", cuda, (device-cuda, sm_20)
// CHK-PHASES-WITH-CUDA: 4: preprocessor, {3}, cuda-cpp-output, (device-cuda, sm_20)
// CHK-PHASES-WITH-CUDA: 5: compiler, {4}, ir, (device-cuda, sm_20)
// CHK-PHASES-WITH-CUDA: 6: backend, {5}, assembler, (device-cuda, sm_20)
// CHK-PHASES-WITH-CUDA: 7: assembler, {6}, object, (device-cuda, sm_20)
// CHK-PHASES-WITH-CUDA: 8: offload, "device-cuda (nvptx64-nvidia-cuda:sm_20)" {7}, object
// CHK-PHASES-WITH-CUDA: 9: offload, "device-cuda (nvptx64-nvidia-cuda:sm_20)" {6}, assembler
// CHK-PHASES-WITH-CUDA: 10: linker, {8, 9}, cuda-fatbin, (device-cuda)
// CHK-PHASES-WITH-CUDA: 11: offload, "host-cuda-openmp (powerpc64le-ibm-linux-gnu)" {2}, "device-cuda (nvptx64-nvidia-cuda)" {10}, ir
// CHK-PHASES-WITH-CUDA: 12: backend, {11}, assembler, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 13: assembler, {12}, object, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 14: linker, {13}, image, (host-cuda-openmp)
// CHK-PHASES-WITH-CUDA: 15: input, "[[INPUT]]", cuda, (device-openmp)
// CHK-PHASES-WITH-CUDA: 16: preprocessor, {15}, cuda-cpp-output, (device-openmp)
// CHK-PHASES-WITH-CUDA: 17: compiler, {16}, ir, (device-openmp)
// CHK-PHASES-WITH-CUDA: 18: offload, "host-cuda-openmp (powerpc64le-ibm-linux-gnu)" {2}, "device-openmp (nvptx64-nvidia-cuda)" {17}, ir
// CHK-PHASES-WITH-CUDA: 19: backend, {18}, assembler, (device-openmp)
// CHK-PHASES-WITH-CUDA: 20: assembler, {19}, object, (device-openmp)
// CHK-PHASES-WITH-CUDA: 21: linker, {20}, image, (device-openmp)
// CHK-PHASES-WITH-CUDA: 22: offload, "host-cuda-openmp (powerpc64le-ibm-linux-gnu)" {14}, "device-openmp (nvptx64-nvidia-cuda)" {21}, image

/// ###########################################################################

/// Check of the commands passed to each tool when using valid OpenMP targets.
/// Here we also check that offloading does not break the use of integrated
/// assembler. It does however preclude the merge of the host compile and
/// backend phases. There are also two offloading specific options:
/// -fopenmp-is-device: will tell the frontend that it will generate code for a
/// target.
/// -fopenmp-host-ir-file-path: specifies the host IR file that can be loaded by
/// the target code generation to gather information about which declaration
/// really need to be emitted.
/// We use -fopenmp-dump-offload-linker-script to dump the linker script and
/// check its contents.
///
// RUN:   %clang -### -fopenmp=libomp -o %t.out -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -fopenmp-dump-offload-linker-script -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-COMMANDS -check-prefix=CHK-LKS -check-prefix=CHK-LKS-REG %s
// RUN:   %clang -### -fopenmp=libomp -o %t.out -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -save-temps -fopenmp-dump-offload-linker-script -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-COMMANDS-ST -check-prefix=CHK-LKS -check-prefix=CHK-LKS-ST %s

// Make sure we are not dumping the script unless the user requested it.
// RUN:   %clang -### -fopenmp=libomp -o %t.out -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-LKS-NODUMP %s
// RUN:   %clang -### -fopenmp=libomp -o %t.out -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -save-temps -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-LKS-NODUMP %s

//
// Check the linker script contains what we expect.
//
// CHK-LKS: /*
// CHK-LKS:                   OpenMP Offload Linker Script
// CHK-LKS:             *** Automatically generated by Clang ***
// CHK-LKS-NODUMP-NOT:  OpenMP Offload Linker Script.
// CHK-LKS: */
// CHK-LKS: TARGET(binary)
// CHK-LKS-REG: INPUT([[T1BIN:.+\.out]])
// CHK-LKS-REG: INPUT([[T2BIN:.+\.out]])
// CHK-LKS-ST: INPUT([[T1BIN:.+\.out-openmp-powerpc64le-ibm-linux-gnu]])
// CHK-LKS-ST: INPUT([[T2BIN:.+\.out-openmp-x86_64-pc-linux-gnu]])
// CHK-LKS: SECTIONS
// CHK-LKS: {
// CHK-LKS:   .omp_offloading.powerpc64le-ibm-linux-gnu :
// CHK-LKS:   ALIGN(0x10)
// CHK-LKS:   {
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.img_start.powerpc64le-ibm-linux-gnu = .);
// CHK-LKS:     [[T1BIN]]
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.img_end.powerpc64le-ibm-linux-gnu = .);
// CHK-LKS:   }
// CHK-LKS:   .omp_offloading.x86_64-pc-linux-gnu :
// CHK-LKS:   ALIGN(0x10)
// CHK-LKS:   {
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.img_start.x86_64-pc-linux-gnu = .);
// CHK-LKS:     [[T2BIN]]
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.img_end.x86_64-pc-linux-gnu = .);
// CHK-LKS:   }
// CHK-LKS:   .omp_offloading.entries :
// CHK-LKS:   ALIGN(0x10)
// CHK-LKS:   SUBALIGN(0x01)
// CHK-LKS:   {
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.entries_begin = .);
// CHK-LKS:     *(.omp_offloading.entries)
// CHK-LKS:     PROVIDE_HIDDEN(.omp_offloading.entries_end = .);
// CHK-LKS:   }
// CHK-LKS: }
// CHK-LKS: INSERT BEFORE .data

//
// Generate host BC file.
//
// CHK-COMMANDS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "c" "
// CHK-COMMANDS-SAME: [[INPUT:[^\\/]+\.c]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-E" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[HOSTPP:[^\\/]+\.i]]" "-x" "c" "
// CHK-COMMANDS-ST-SAME: [[INPUT:[^\\/]+\.c]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

//
// Compile for the powerpc device.
//
// CHK-COMMANDS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-pic-level" "2" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[T1OBJ:[^\\/]+\.o]]" "-x" "c" "{{.*}}[[INPUT]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[T1BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T1OBJ]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-E" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T1PP:[^\\/]+\.i]]" "-x" "c" "{{.*}}[[INPUT]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-pic-level" "2" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T1BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T1ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T1BC]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-ibm-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T1OBJ:[^\\/]+\.o]]" "{{.*}}[[T1ASM]]"
// CHK-COMMANDS-ST: ld{{(\.exe)?}}" {{.*}}"-shared" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T1BIN:[^\\/]+\.out-openmp-powerpc64le-ibm-linux-gnu]]" {{.*}}"{{.*}}[[T1OBJ]]"
//
// Compile for the x86 device.
//
// CHK-COMMANDS: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-pic-level" "2" {{.*}}"-fopenmp"  {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[T2OBJ:[^\\/]+\.o]]" "-x" "c" "{{.*}}[[INPUT]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[T2BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T2OBJ]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-E" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T2PP:[^\\/]+\.i]]" "-x" "c" "{{.*}}[[INPUT]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-pic-level" "2" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T2BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T2ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T2BC]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1as" "-triple" "x86_64-pc-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T2OBJ:[^\\/]+\.o]]" "{{.*}}[[T2ASM]]"
// CHK-COMMANDS-ST: ld{{(\.exe)?}}" {{.*}}"-shared" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[T2BIN:[^\\/]+\.out-openmp-x86_64-pc-linux-gnu]]" {{.*}}"{{.*}}[[T2OBJ]]"

//
// Generate host object from the BC file and link using the linker script.
//
// CHK-COMMANDS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-COMMANDS-SAME: [[HOSTBIN:[^\\/]+\.out]]"  {{.*}}"-lomptarget" {{.*}}"-T" "
// CHK-COMMANDS-SAME: [[HOSTLK:[^\\/]+\.lk]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[HOSTASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-COMMANDS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-unknown-linux" "-filetype" "obj" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "{{.*}}[[HOSTASM]]"
// CHK-COMMANDS-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-COMMANDS-ST-SAME: [[HOSTBIN:[^\\/]+\.out]]"  {{.*}}"-lomptarget" {{.*}}"-T" "
// CHK-COMMANDS-ST-SAME: [[HOSTLK:[^\\/]+\.lk]]"


/// ###########################################################################

/// Check separate compilation with offloading - bundling actions
// RUN:   %clang -### -ccc-print-phases -fopenmp=libomp -c -o %t.o %S/Input/in.so -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-BUACTIONS %s

// CHK-BUACTIONS: 0: input, "[[INPUT:.+\.c]]", c, (host-openmp)
// CHK-BUACTIONS: 1: preprocessor, {0}, cpp-output, (host-openmp)
// CHK-BUACTIONS: 2: compiler, {1}, ir, (host-openmp)
// CHK-BUACTIONS: 3: input, "[[INPUT]]", c, (device-openmp)
// CHK-BUACTIONS: 4: preprocessor, {3}, cpp-output, (device-openmp)
// CHK-BUACTIONS: 5: compiler, {4}, ir, (device-openmp)
// CHK-BUACTIONS: 6: offload, "host-openmp (powerpc64le-unknown-linux)" {2}, "device-openmp (powerpc64le-ibm-linux-gnu)" {5}, ir
// CHK-BUACTIONS: 7: backend, {6}, assembler, (device-openmp)
// CHK-BUACTIONS: 8: assembler, {7}, object, (device-openmp)
// CHK-BUACTIONS: 9: offload, "device-openmp (powerpc64le-ibm-linux-gnu)" {8}, object
// CHK-BUACTIONS: 10: input, "[[INPUT]]", c, (device-openmp)
// CHK-BUACTIONS: 11: preprocessor, {10}, cpp-output, (device-openmp)
// CHK-BUACTIONS: 12: compiler, {11}, ir, (device-openmp)
// CHK-BUACTIONS: 13: offload, "host-openmp (powerpc64le-unknown-linux)" {2}, "device-openmp (x86_64-pc-linux-gnu)" {12}, ir
// CHK-BUACTIONS: 14: backend, {13}, assembler, (device-openmp)
// CHK-BUACTIONS: 15: assembler, {14}, object, (device-openmp)
// CHK-BUACTIONS: 16: offload, "device-openmp (x86_64-pc-linux-gnu)" {15}, object
// CHK-BUACTIONS: 17: backend, {2}, assembler, (host-openmp)
// CHK-BUACTIONS: 18: assembler, {17}, object, (host-openmp)
// CHK-BUACTIONS: 19: clang-offload-bundler, {9, 16, 18}, object, (host-openmp)

/// ###########################################################################

/// Check separate compilation with offloading - unbundling actions
// RUN:   touch %t.i
// RUN:   %clang -### -ccc-print-phases -fopenmp=libomp -o %t.out -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBACTIONS %s

// CHK-UBACTIONS: 0: input, "somelib", object, (host-openmp)
// CHK-UBACTIONS: 1: input, "[[INPUT:.+\.i]]", cpp-output, (host-openmp)
// CHK-UBACTIONS: 2: clang-offload-unbundler, {1}, cpp-output, (host-openmp)
// CHK-UBACTIONS: 3: compiler, {2}, ir, (host-openmp)
// CHK-UBACTIONS: 4: backend, {3}, assembler, (host-openmp)
// CHK-UBACTIONS: 5: assembler, {4}, object, (host-openmp)
// CHK-UBACTIONS: 6: linker, {0, 5}, image, (host-openmp)
// CHK-UBACTIONS: 7: input, "somelib", object, (device-openmp)
// CHK-UBACTIONS: 8: compiler, {2}, ir, (device-openmp)
// CHK-UBACTIONS: 9: offload, "host-openmp (powerpc64le-unknown-linux)" {3}, "device-openmp (powerpc64le-ibm-linux-gnu)" {8}, ir
// CHK-UBACTIONS: 10: backend, {9}, assembler, (device-openmp)
// CHK-UBACTIONS: 11: assembler, {10}, object, (device-openmp)
// CHK-UBACTIONS: 12: linker, {7, 11}, image, (device-openmp)
// CHK-UBACTIONS: 13: input, "somelib", object, (device-openmp)
// CHK-UBACTIONS: 14: compiler, {2}, ir, (device-openmp)
// CHK-UBACTIONS: 15: offload, "host-openmp (powerpc64le-unknown-linux)" {3}, "device-openmp (x86_64-pc-linux-gnu)" {14}, ir
// CHK-UBACTIONS: 16: backend, {15}, assembler, (device-openmp)
// CHK-UBACTIONS: 17: assembler, {16}, object, (device-openmp)
// CHK-UBACTIONS: 18: linker, {13, 17}, image, (device-openmp)
// CHK-UBACTIONS: 19: offload, "host-openmp (powerpc64le-unknown-linux)" {6}, "device-openmp (powerpc64le-ibm-linux-gnu)" {12}, "device-openmp (x86_64-pc-linux-gnu)" {18}, image

/// ###########################################################################

/// Check separate compilation with offloading - unbundling/bundling actions
// RUN:   touch %t.i
// RUN:   %clang -### -ccc-print-phases -fopenmp=libomp -c -o %t.o -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBUACTIONS %s

// CHK-UBUACTIONS: 0: input, "[[INPUT:.+\.i]]", cpp-output, (host-openmp)
// CHK-UBUACTIONS: 1: clang-offload-unbundler, {0}, cpp-output, (host-openmp)
// CHK-UBUACTIONS: 2: compiler, {1}, ir, (host-openmp)
// CHK-UBUACTIONS: 3: compiler, {1}, ir, (device-openmp)
// CHK-UBUACTIONS: 4: offload, "host-openmp (powerpc64le-unknown-linux)" {2}, "device-openmp (powerpc64le-ibm-linux-gnu)" {3}, ir
// CHK-UBUACTIONS: 5: backend, {4}, assembler, (device-openmp)
// CHK-UBUACTIONS: 6: assembler, {5}, object, (device-openmp)
// CHK-UBUACTIONS: 7: offload, "device-openmp (powerpc64le-ibm-linux-gnu)" {6}, object
// CHK-UBUACTIONS: 8: compiler, {1}, ir, (device-openmp)
// CHK-UBUACTIONS: 9: offload, "host-openmp (powerpc64le-unknown-linux)" {2}, "device-openmp (x86_64-pc-linux-gnu)" {8}, ir
// CHK-UBUACTIONS: 10: backend, {9}, assembler, (device-openmp)
// CHK-UBUACTIONS: 11: assembler, {10}, object, (device-openmp)
// CHK-UBUACTIONS: 12: offload, "device-openmp (x86_64-pc-linux-gnu)" {11}, object
// CHK-UBUACTIONS: 13: backend, {2}, assembler, (host-openmp)
// CHK-UBUACTIONS: 14: assembler, {13}, object, (host-openmp)
// CHK-UBUACTIONS: 15: clang-offload-bundler, {7, 12, 14}, object, (host-openmp)

/// ###########################################################################

/// Check separate compilation with offloading - bundling jobs construct
// RUN:   %clang -### -fopenmp=libomp -c -o %t.o -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-BUJOBS %s
// RUN:   %clang -### -fopenmp=libomp -c -o %t.o -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %s -save-temps -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-BUJOBS-ST %s

// Create host BC.
// CHK-BUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "c" "
// CHK-BUJOBS-SAME: [[INPUT:[^\\/]+\.c]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-E"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[HOSTPP:[^\\/]+\.i]]" "-x" "c" "
// CHK-BUJOBS-ST-SAME: [[INPUT:[^\\/]+\.c]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

// Create target 1 object.
// CHK-BUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-SAME: [[T1OBJ:[^\\/]+\.o]]" "-x" "c" "{{.*}}[[INPUT]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-E" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T1PP:[^\\/]+\.i]]" "-x" "c" "{{.*}}[[INPUT]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T1BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T1ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T1BC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-ibm-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T1OBJ:[^\\/]+\.o]]" "{{.*}}[[T1ASM]]"

// Create target 2 object.
// CHK-BUJOBS: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-SAME: [[T2OBJ:[^\\/]+\.o]]" "-x" "c" "{{.*}}[[INPUT]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-E" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T2PP:[^\\/]+\.i]]" "-x" "c" "{{.*}}[[INPUT]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T2BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T2ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T2BC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "x86_64-pc-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[T2OBJ:[^\\/]+\.o]]" "{{.*}}[[T2ASM]]"

// Create host object and bundle.
// CHK-BUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS: clang-offload-bundler{{.*}}" "-type=o"{{.*}}"-targets=openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu,host-powerpc64le-unknown-linux" "-outputs=
// CHK-BUJOBS-SAME: [[RES:[^\\/]+\.o]]" "-inputs={{.*}}[[T1OBJ]],{{.*}}[[T2OBJ]],{{.*}}[[HOSTOBJ]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[HOSTASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-BUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-unknown-linux" "-filetype" "obj" {{.*}}"-o" "
// CHK-BUJOBS-ST-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "{{.*}}[[HOSTASM]]"
// CHK-BUJOBS-ST: clang-offload-bundler{{.*}}" "-type=o"{{.*}}"-targets=openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu,host-powerpc64le-unknown-linux" "-outputs=
// CHK-BUJOBS-ST-SAME: [[RES:[^\\/]+\.o]]" "-inputs={{.*}}[[T1OBJ]],{{.*}}[[T2OBJ]],{{.*}}[[HOSTOBJ]]"

/// ###########################################################################

/// Check separate compilation with offloading - unbundling jobs construct
// RUN:   touch %t.i
// RUN:   %clang -###  -fopenmp=libomp -o %t.out -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBJOBS %s
// RUN:   %clang -### -fopenmp=libomp -o %t.out -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -save-temps -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBJOBS-ST %s
// RUN:   touch %t.o
// RUN:   %clang -###  -fopenmp=libomp -o %t.out -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.o -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBJOBS2 %s
// RUN:   %clang -### -fopenmp=libomp -o %t.out -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.o %S/Inputs/in.so -save-temps -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBJOBS2-ST %s

// Unbundle and create host BC.
// CHK-UBJOBS: clang-offload-bundler{{.*}}" "-type=i" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBJOBS-SAME: [[INPUT:[^\\/]+\.i]]" "-outputs=
// CHK-UBJOBS-SAME: [[HOSTPP:[^\\/]+\.i]],
// CHK-UBJOBS-SAME: [[T1PP:[^\\/]+\.i]],
// CHK-UBJOBS-SAME: [[T2PP:[^\\/]+\.i]]" "-unbundle"
// CHK-UBJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"
// CHK-UBJOBS-ST: clang-offload-bundler{{.*}}" "-type=i" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBJOBS-ST-SAME: [[INPUT:[^\\/]+\.i]]" "-outputs=
// CHK-UBJOBS-ST-SAME: [[HOSTPP:[^\\/,]+\.i]],
// CHK-UBJOBS-ST-SAME: [[T1PP:[^\\/,]+\.i]],
// CHK-UBJOBS-ST-SAME: [[T2PP:[^\\/,]+\.i]]" "-unbundle"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

// Create target 1 object.
// CHK-UBJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[T1OBJ:[^\\/]+\.o]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[T1BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T1OBJ]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T1BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T1ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T1BC]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-ibm-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T1OBJ:[^\\/]+\.o]]" "{{.*}}[[T1ASM]]"
// CHK-UBJOBS-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T1BIN:[^\\/]+\.out-openmp-powerpc64le-ibm-linux-gnu]]" {{.*}}"{{.*}}[[T1OBJ]]"

// Create target 2 object.
// CHK-UBJOBS: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[T2OBJ:[^\\/]+\.o]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[T2BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T2OBJ]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T2BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T2ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T2BC]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "x86_64-pc-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T2OBJ:[^\\/]+\.o]]" "{{.*}}[[T2ASM]]"
// CHK-UBJOBS-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[T2BIN:[^\\/]+\.out-openmp-x86_64-pc-linux-gnu]]" {{.*}}"{{.*}}[[T2OBJ]]"

// Create binary.
// CHK-UBJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-SAME: [[HOSTBIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[HOSTOBJ]]" {{.*}}"-T" "
// CHK-UBJOBS-SAME: [[LKS:[^\\/]+\.lk]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[HOSTASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-UBJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-unknown-linux" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "{{.*}}[[HOSTASM]]"
// CHK-UBJOBS-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS-ST-SAME: [[HOSTBIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[HOSTOBJ]]" {{.*}}"-T" "
// CHK-UBJOBS-ST-SAME: [[LKS:[^\\/]+\.lk]]"

// Unbundle object file.
// CHK-UBJOBS2: clang-offload-bundler{{.*}}" "-type=o" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBJOBS2-SAME: [[INPUT:[^\\/]+\.o]]" "-outputs=
// CHK-UBJOBS2-SAME: [[HOSTOBJ:[^\\/]+\.o]],
// CHK-UBJOBS2-SAME: [[T1OBJ:[^\\/]+\.o]],
// CHK-UBJOBS2-SAME: [[T2OBJ:[^\\/]+\.o]]" "-unbundle"
// CHK-UBJOBS2: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-SAME: [[T1BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T1OBJ]]"
// CHK-UBJOBS2: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-SAME: [[T2BIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[T2OBJ]]"
// CHK-UBJOBS2: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-SAME: [[HOSTBIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[HOSTOBJ]]" {{.*}}"-T" "
// CHK-UBJOBS2-SAME: [[LKS:[^\\/]+\.lk]]"
// CHK-UBJOBS2-ST-NOT: clang-offload-bundler{{.*}}in.so
// CHK-UBJOBS2-ST: clang-offload-bundler{{.*}}" "-type=o" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBJOBS2-ST-SAME: [[INPUT:[^\\/]+\.o]]" "-outputs=
// CHK-UBJOBS2-ST-SAME: [[HOSTOBJ:[^\\/,]+\.o]],
// CHK-UBJOBS2-ST-SAME: [[T1OBJ:[^\\/,]+\.o]],
// CHK-UBJOBS2-ST-SAME: [[T2OBJ:[^\\/,]+\.o]]" "-unbundle"
// CHK-UBJOBS2-ST-NOT: clang-offload-bundler{{.*}}in.so
// CHK-UBJOBS2-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-ST-SAME: [[T1BIN:[^\\/]+\.out-openmp-powerpc64le-ibm-linux-gnu]]" {{.*}}"{{.*}}[[T1OBJ]]"
// CHK-UBJOBS2-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-ST-SAME: [[T2BIN:[^\\/]+\.out-openmp-x86_64-pc-linux-gnu]]" {{.*}}"{{.*}}[[T2OBJ]]"
// CHK-UBJOBS2-ST: ld{{(\.exe)?}}" {{.*}}"-o" "
// CHK-UBJOBS2-ST-SAME: [[HOSTBIN:[^\\/]+\.out]]" {{.*}}"{{.*}}[[HOSTOBJ]]" {{.*}}"-T" "
// CHK-UBJOBS2-ST-SAME: [[LKS:[^\\/]+\.lk]]"

/// ###########################################################################

/// Check separate compilation with offloading - unbundling/bundling jobs
/// construct
// RUN:   touch %t.i
// RUN:   %clang -### -fopenmp=libomp -c %t.o -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBUJOBS %s
// RUN:   %clang -### -fopenmp=libomp -c %t.o -lsomelib -target powerpc64le-linux -fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu %t.i -save-temps -no-canonical-prefixes 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-UBUJOBS-ST %s

// Unbundle and create host BC.
// CHK-UBUJOBS: clang-offload-bundler{{.*}}" "-type=i" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBUJOBS-SAME: [[INPUT:[^\\/]+\.i]]" "-outputs=
// CHK-UBUJOBS-SAME: [[HOSTPP:[^\\/]+\.i]],
// CHK-UBUJOBS-SAME: [[T1PP:[^\\/]+\.i]],
// CHK-UBUJOBS-SAME: [[T2PP:[^\\/]+\.i]]" "-unbundle"
// CHK-UBUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

// CHK-UBUJOBS-ST: clang-offload-bundler{{.*}}" "-type=i" "-targets=host-powerpc64le-unknown-linux,openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu" "-inputs=
// CHK-UBUJOBS-ST-SAME: [[INPUT:[^\\/]+\.i]]" "-outputs=
// CHK-UBUJOBS-ST-SAME: [[HOSTPP:[^\\/,]+\.i]],
// CHK-UBUJOBS-ST-SAME: [[T1PP:[^\\/,]+\.i]],
// CHK-UBUJOBS-ST-SAME: [[T2PP:[^\\/,]+\.i]]" "-unbundle"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc"  {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[HOSTBC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[HOSTPP]]" "-fopenmp-targets=powerpc64le-ibm-linux-gnu,x86_64-pc-linux-gnu"

// Create target 1 object.
// CHK-UBUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-SAME: [[T1OBJ:[^\\/]+\.o]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T1BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T1PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-ibm-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T1ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T1BC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-ibm-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T1OBJ:[^\\/]+\.o]]" "{{.*}}[[T1ASM]]"

// Create target 2 object.
// CHK-UBUJOBS: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-SAME: [[T2OBJ:[^\\/]+\.o]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-emit-llvm-bc" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T2BC:[^\\/]+\.bc]]" "-x" "cpp-output" "{{.*}}[[T2PP]]" "-fopenmp-is-device" "-fopenmp-host-ir-file-path" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "x86_64-pc-linux-gnu" "-aux-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T2ASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[T2BC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "x86_64-pc-linux-gnu" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[T2OBJ:[^\\/]+\.o]]" "{{.*}}[[T2ASM]]"

// Create binary.
// CHK-UBUJOBS: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-emit-obj" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS: clang-offload-bundler{{.*}}" "-type=o" "-targets=openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu,host-powerpc64le-unknown-linux" "-outputs=
// CHK-UBUJOBS-SAME: [[RES:[^\\/]+\.o]]" "-inputs={{.*}}[[T1OBJ]],{{.*}}[[T2OBJ]],{{.*}}[[HOSTOBJ]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1" "-triple" "powerpc64le-unknown-linux" "-S" {{.*}}"-fopenmp" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[HOSTASM:[^\\/]+\.s]]" "-x" "ir" "{{.*}}[[HOSTBC]]"
// CHK-UBUJOBS-ST: clang{{.*}}" "-cc1as" "-triple" "powerpc64le-unknown-linux" "-filetype" "obj" {{.*}}"-o" "
// CHK-UBUJOBS-ST-SAME: [[HOSTOBJ:[^\\/]+\.o]]" "{{.*}}[[HOSTASM]]"
// CHK-UBUJOBS-ST: clang-offload-bundler{{.*}}" "-type=o" "-targets=openmp-powerpc64le-ibm-linux-gnu,openmp-x86_64-pc-linux-gnu,host-powerpc64le-unknown-linux" "-outputs=
// CHK-UBUJOBS-ST-SAME: [[RES:[^\\/]+\.o]]" "-inputs={{.*}}[[T1OBJ]],{{.*}}[[T2OBJ]],{{.*}}[[HOSTOBJ]]"

/// ###########################################################################

/// Check -fopenmp-is-device is passed when compiling for the device.
// RUN:   %clang -### -no-canonical-prefixes -target powerpc64le-linux -fopenmp=libomp -fopenmp-targets=powerpc64le-ibm-linux-gnu %s 2>&1 \
// RUN:   | FileCheck -check-prefix=CHK-FOPENMP-IS-DEVICE %s

// CHK-FOPENMP-IS-DEVICE: clang{{.*}} "-aux-triple" "powerpc64le-unknown-linux" {{.*}}.c" "-fopenmp-is-device" "-fopenmp-host-ir-file-path"
