# MaxEVA: Maximizing the Efficiency of Matrix Multiplication on Versal AI Engine

This is the official github repo of MaxEVA paper published at FPT2023 Conference.

Available preprint on arxiv: **XXXXXXXXXXXXXXXXXXXXXX**


## Requirements

It is recommended to review the [AMD/Xilinx AI Engine blog series](https://support.xilinx.com/s/question/0D52E00006xR6iXSAS/ai-engine-blog-series?language=en_US) before running this tutorial.

This tutorial requires the following:
1. The Vitis 2022.1 software platform installed on a supported Linux OS.
2. A valid license for the AI Engine tools (licenses can be generated from AMD user account).
3. The VCK190 base platform downloaded from the following [link](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-platforms/2022-1.html).


## Compile & Run a MaXEVA Configuration

1. Create a new application project (e.g., named `<project_name_dir>`) as shown [here](https://support.xilinx.com/s/article/1184031?language=en_US).

2. Replace the contents of `<project_name_dir>/data` and `<project_name_dir>/src` directories with a MaxEVA configuration, e.g., [`Pattern1_int8/data`](Pattern1_int8/data) and [`Pattern1_int8/src`](Pattern1_int8/src), respectively for pattern `P1`, `int8` MaxEVA configuration.

3. Right click on `<project_name_dir> [ aiengine ]` and select `C/C++ Build Settings` as shown in the figure below:
   
   <img src="images/prj_build_settings.png" width=30% height=30%>

   At `Compiler` Settings > `Mapper Options` invoke buffer optimizations level 8, i.e., `BufferOptLevel8`:

   <img src="images/compiler_mapper.png" width=60% height=60%>

   This flag will optimize buffer placement and will prevent buffer conflicts where possible, leading to higher performance. More details can be found at AMD documentation [UG1076](https://docs.xilinx.com/r/2022.1-English/ug1076-ai-engine-environment/Memory-Stalls).

4.  Right click on `<project_name_dir> [ aiengine ]` and select `Build Project`. This step will build the project for HW Emulation. Make sure the Active build Configuration is set to `Emulation-AIE` as shown [here](https://support.xilinx.com/s/article/1199671?language=en_US).

5. Generate input and golden data utilizing the `generate_golden_{int8, fp32}.cpp` C++ script (example below shows the `int8` case):

   ```bash
   cd <project_name_dir>/data
   g++ -o generate_golden_int8 generate_golden_int8.cpp
   ./generate_golden_int8
   ```

   Note: This script was tested with g++ 9.4.0 compiler version.

6. When build is finished, right click on `<project_name_dir> [ aiengine ]` > `Run As` > `Launch AIE Simulator`, to simulate the project.

7. When simulation is finished, run the [`performance_calc.py`](performance_calc.py) Python script to calculate the MaxEVA throughput. Below is an example of `P1`, `int8`:

   ```bash
   python3 performance_calc.py --project_dir=<absolute_path_to_project_name_dir> --single_M=32 --single_K=128 --single_N=32 --mult_X=13 --mult_Y=4 --mult_Z=6 --precision=int8
   ```

   In this example the `int8` **X**x**Y**x**Z** = **13**x**4**x**6** MaxEVA configuration has been compiled (Table II on manuscript), with single AIE kernel size **M**x**K**x**N** = **32**x**128**x**32** (Table I on manuscript).

   Note: To compile and run a new MaxEVA configuration of the same pattern, e.g., **11**x**4**x**7**, modify the parameters `mult_{X, Y, Z}` at [`Pattern1_int8/src/kernels/include.h`](Pattern1_int8/src/kernels/include.h) accordingly (see Table II on manuscript for more details), and repeat steps **4-7**.
   
