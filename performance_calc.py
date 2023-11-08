import argparse
import glob, os
from statistics import mean, variance, stdev



# Initialize the Parser
parser = argparse.ArgumentParser(description ='Analyze AIE results.')

parser.add_argument('--project_dir', type=str, help='Give the absolute project directory path. Emulation output data at: /<project_dir>/Emulation-AIE/aiesimulator_output/data')
parser.add_argument('--single_M', type=int, help='dim M of single MatMul kernel')
parser.add_argument('--single_K', type=int, help='dim K of single MatMul kernel')
parser.add_argument('--single_N', type=int, help='dim N of single MatMul kernel')
parser.add_argument('--mult_X', type=int, help='X MaxEVA parameter')
parser.add_argument('--mult_Y', type=int, help='Y MaxEVA parameter')
parser.add_argument('--mult_Z', type=int, help='Z MaxEVA parameter')
parser.add_argument('--precision', type=str, help='Give either "int8" or "fp32"')


args = parser.parse_args()

project_dir = args.project_dir

single_M = args.single_M
single_K = args.single_K
single_N = args.single_N

mult_X = args.mult_X
mult_Y = args.mult_Y
mult_Z = args.mult_Z

precision = args.precision


global_diff_list = []


for filename in glob.glob(project_dir + "/Emulation-AIE/aiesimulator_output/data/matC*.txt"):


    with open(filename) as file:

        # list that keeps the time of tlast on each file separetely
        file_tlast_list = []

        prevline = ""
        for line in file:
            if "TLAST" in line:
                time_list = prevline.split()

                if (time_list[2] == "ns"):
                    file_tlast_list.append(float(time_list[1]))
                elif (time_list[2] == "ps"):
                    file_tlast_list.append(float(time_list[1])/1000)
                elif (time_list[2] == "us"):
                    file_tlast_list.append(float(time_list[1])*1000)
                else:
                    print(time_list[2])
                    print("not ns or ps... exiting")
                    exit(1)

            prevline = line


        prev_time = file_tlast_list[0]
        file_diff_list = []
        for i in range(len(file_tlast_list)):
            
            if (i > 0):
                file_diff_list.append(file_tlast_list[i]-prev_time)
                global_diff_list.append(file_diff_list[i-1])
                
            prev_time = file_tlast_list[i]




Throughput = 2*(mult_X*single_M)*(mult_Y*single_K)*(mult_Z*single_N) / mean(global_diff_list)


print("")
print("Matrix size:", mult_X*single_M, "x", mult_Y*single_K, "x", mult_Z*single_N)

if (precision == "int8"):
    print("Precision: int8")
    print("Throughput:",round(Throughput/1000, 5), "TOPs")
elif (precision == "fp32"):
    print("Precision: fp32")
    print("Throughput:",round(Throughput, 5), "GFLOPs")
else:
    print("Give precision as either 'int8' or 'fp32'")


