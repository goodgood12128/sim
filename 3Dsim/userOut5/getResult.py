import os
import re
import argparse
import csv

def extract_information(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

        # match = re.search(r'.*normal page read count :[ \t]*(\d+)[ \t]*\n.*mutli plane one shot read count :[ \t]*(\d+)[ \t]*\n.*one shot read count :[ \t]*(\d+)[ \t]*\n.*multi-plane read count:[ \t]*(\d+)[ \t]*\n.*mutli plane SOML read count', content, re.S)
        match = re.search(r'.*normal page read count :[ \t]*(\d+)[ \t]*\n.*mutli plane one shot read count :[ \t]*(\d+)[ \t]*\n.*one shot read count :[ \t]*(\d+)[ \t]*\n.*multi-plane read count:[ \t]*(\d+)[ \t]*\n.*mutli plane SOML read count :[ \t]*(\d+)[ \t]*\n.*SOML read count :[ \t]*(\d+)[ \t]*\n.*read request count:[ \t]*(\d+)[ \t]*\n.*write request count:[ \t]*(\d+)[ \t]*\n.*read request average size:[ \t]*(\d+\.\d+)[ \t]*\n.*write request average size:[ \t]*(\d+\.\d+)[ \t]*\n.*read request average response time:[ \t]*(\d+)[ \t]*\n.*write request average response time:[ \t]*(\d+)[ \t]*', content,re.S)
        
        if match == None:
            print(match)

        if match:
            return {
                'filename': os.path.basename(file_path),
                'Normal_page_read_count': int(match.group(1)),
                'Multi-plane_one_shot_read_count': int(match.group(2)),
                'One_shot_read_count': int(match.group(3)),
                'Multi-plane_read_count': int(match.group(4)),
                'Multi-plane_SOML_read_count': int(match.group(5)),
                'SOML_read_count': int(match.group(6)),
                'Write_request_count': int(match.group(7)),
                'Write_request_count': int(match.group(8)),
                'Write_request_average_size': float(match.group(9)),
                'Write_request_average_size': float(match.group(10)),
                'Read_request_average_response_time': int(match.group(11)),
                'Write_request_average_response_time': int(match.group(12))
            }
    return None

def main():
    # 创建参数解析器
    parser = argparse.ArgumentParser(description="Extract information from userOut files and save to CSV.")

    # 添加输入文件夹和输出CSV文件参数
    parser.add_argument("-i", "--input_folder", required=True, help="Input folder path")
    parser.add_argument("-o", "--output_csv", required=True, help="Output CSV file path")

    # 解析命令行参数
    args = parser.parse_args()

    # 用于存储提取的信息的列表
    results = []

    # 遍历文件夹中的文件
    for filename in os.listdir(args.input_folder):
        if filename.endswith(".out"):
            file_path = os.path.join(args.input_folder, filename)
            info = extract_information(file_path)
            if info:
                results.append(info)

    # 将提取的信息写入CSV文件
    with open(args.output_csv, 'w', newline='') as csv_file:
        fieldnames = results[0].keys() if results else []
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)

        writer.writeheader()
        writer.writerows(results)

    print(f"提取的信息已保存到 {args.output_csv}")

if __name__ == "__main__":
    main()
