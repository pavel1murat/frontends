#!/usr/bin/env python3
# python program_tracker_fpgas.sh 
import argparse
import subprocess

CFO_VER      = "CFO2026Aug03_20_22.1"
CFO_BIT_FILE = f"~/CFO_Firmware/{CFO_VER}/CFO.bit"

DTC_VER = "DTC2026Aug08_19_22.1_6ROC"
DTC_BIT_FILE = f"~/DTC_Firmware/{DTC_VER}/DTC.bit"


def run_ssh(node, remote_command, execute=False):
    command = ["ssh", "-K", "-t", node, remote_command]

    print(" ".join(command))

    if execute:
        subprocess.run(command, check=True)


#------------------------------------------------------------------------------
def program_cfos(execute=False):
    remote_command = (
        f"~/program_both_DTCs_AL9.sh {CFO_BIT_FILE} ; "
        f"sudo ~/reset_PCIe_AL9.sh"
    )

    print(remote_command)
    run_ssh("mu2e-calo-13", remote_command, execute)

#------------------------------------------------------------------------------
def program_dtcs(first, last, execute=False):
    for i in range(first, last + 1):
        node = f"mu2e-trk-{i:02d}"

        print(f"------------------------------ reprogramming {node}")

        remote_command = (
            f"~/program_all_FPGA_AL9.sh {DTC_BIT_FILE} ; sudo ~/reset_PCIe_AL9.sh"
        )

        # mu2e-trk-02 also requires mu2e-trk-01 to be reset
        if node == "mu2e-trk-02":
            remote_command += (" ; ssh -K -t mu2e-trk-01 sudo ~/reset_PCIe_AL9.sh" )

        run_ssh(node, remote_command, execute)


def parse_range(value):
    """Parse N or N:M, for example 02 or 02:18."""
    parts = value.split(":")

    if len(parts) == 1:
        first = last = int(parts[0])
    elif len(parts) == 2:
        first = int(parts[0])
        last = int(parts[1])
    else:
        raise argparse.ArgumentTypeError(
            "range must have the form N or N:M"
        )

    if first > last:
        raise argparse.ArgumentTypeError(
            "first node must not be greater than last node"
        )

    return first, last

#------------------------------------------------------------------------------
def main():
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--nodes"   , default="02:18", help="node range, for example 02:18 or 05",)
    parser.add_argument("--cfos"    , action="store_true", help="also program the CFOs",  )
    parser.add_argument("--dtcs"    , action="store_true", help="program the DTCs",  )
    parser.add_argument("--execute" , action="store_true", help="execute the SSH commands; otherwise only print them", )

    args = parser.parse_args()
    first, last = parse_range(args.nodes)

    if args.cfos:
        print("... reprogramming the CFOs")
        program_cfos(args.execute)

    if args.dtcs:
        print("... reprogramming all DTCs")
        program_dtcs(first, last, args.execute)


if __name__ == "__main__":
    main()
