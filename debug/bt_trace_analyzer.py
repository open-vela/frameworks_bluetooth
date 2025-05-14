############################################################################
# Copyright (C) 2025 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################
import re
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
from pathlib import Path

def parse_args():
    parser = argparse.ArgumentParser(description='Latency log analyzer with tag support')
    parser.add_argument('--log', required=True,
                      help='Input log file path')
    parser.add_argument('--output-img', default='latency_by_tag.png',
                      help='Output image path (default: latency_by_tag.png)')
    parser.add_argument('--output-report',
                      help='Output report file path (optional)')
    return parser.parse_args()

def parse_log_line(line):
    pattern = r"\[TAG=([^$$]+)\]\[TS=(\d+)\]\[LAT=(\d+)us\]"

    if match := re.search(pattern, line):
        return {
            "tag": match.group(1),
            "timestamp": datetime.fromtimestamp(int(match.group(2))/1e9),
            "latency": int(match.group(3))
        }
    return None

def load_log_data(log_path):
    try:
        with open(log_path, 'r') as f:
            records = []
            for line in f:
                if record := parse_log_line(line.strip()):
                    records.append(record)
            return pd.DataFrame(records)
    except FileNotFoundError:
        raise SystemExit(f"Error: Log file {log_path} not found")


def generate_latency_plot(df, output_path):
    plt.figure(figsize=(15, 8))
    colors = plt.cm.tab10.colors

    for idx, (tag, group) in enumerate(df.groupby('tag')):
        group = group.sort_values('timestamp')
        plt.plot(group['timestamp'], group['latency'],
                color=colors[idx % 10],
                marker='o', markersize=3,
                linestyle='-', linewidth=1,
                label=tag)

    plt.title('Latency Timeline by Tag')
    plt.xlabel('Timestamp')
    plt.ylabel('Latency (μs)')
    plt.legend(loc='upper left', bbox_to_anchor=(1, 1))
    plt.grid(True, alpha=0.3)
    plt.xticks(rotation=45)
    plt.tight_layout()

    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    plt.close()

def generate_stat_report(df, output_path=None):
    stats = []
    for tag, group in df.groupby('tag'):
        desc = group['latency'].describe(percentiles=[.5, .95, .99])
        stats.append({
            'Tag': tag,
            'Count': desc['count'],
            'Mean': desc['mean'],
            'Min': desc['min'],
            '50%': desc['50%'],
            '95%': desc['95%'],
            '99%': desc['max'],
            'Max': desc['max']
        })

    report_df = pd.DataFrame(stats)
    report_str = report_df.to_string(index=False, float_format='%.2f')

    if output_path:
        Path(output_path).parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w') as f:
            f.write("=== Latency Statistics Report ===\n")
            f.write(report_str)
        print(f"Report saved to {output_path}")
    else:
        print("\n" + report_str)

def main():
    args = parse_args()

    df = load_log_data(args.log)
    if df.empty:
        raise SystemExit("Error: No valid records found in log file")

    generate_latency_plot(df, args.output_img)
    print(f"Visualization saved to {args.output_img}")

    if args.output_report:
        generate_stat_report(df, args.output_report)
    else:
        generate_stat_report(df)

if __name__ == "__main__":
    main()