import os
import subprocess
import argparse

def run_simulation(page_parameters, input_folder, output_folder):
    # 获取输入文件夹中的所有文件
    trace_files = [file for file in os.listdir(input_folder) if os.path.isfile(os.path.join(input_folder, file))]

    # 创建输出文件夹
    os.makedirs(output_folder, exist_ok=True)

    # 遍历每个文件并执行命令
    for trace_file in trace_files:
        input_filepath = os.path.join(input_folder, trace_file)
        output_filepath = os.path.join(output_folder, trace_file + ".out")
        
        # 构建命令
        command = ["./My3Dsim", "-p", page_parameters, "-t", input_filepath, "-o", output_filepath]
        
        # 执行命令
        try:
            subprocess.run(command, check=True)
            print(f"执行成功: {trace_file}")
        except subprocess.CalledProcessError as e:
            print(f"执行失败: {trace_file}. 错误消息: {e}")

if __name__ == "__main__":
    # 创建参数解析器
    parser = argparse.ArgumentParser(description="Run simulation on trace files.")
    
    parser.add_argument("-p", "--page_parameters", required=True, help="page_parameters")
    # 添加输入文件夹参数
    parser.add_argument("-i", "--input_folder", required=True, help="Input folder path")
    
    # 添加输出文件夹参数
    parser.add_argument("-o", "--output_folder", required=True, help="Output folder path")
    
    # 解析命令行参数
    args = parser.parse_args()

    # 运行模拟
    run_simulation(args.page_parameters, args.input_folder, args.output_folder)
