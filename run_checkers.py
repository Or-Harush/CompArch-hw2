#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function
import os
import glob
import sys

# ==========================================
# CONFIGURATION
TEST_DIR = "cacheyCheckers"
# ==========================================

# ANSI Colors
class C:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    FUCHSIA = '\033[95m'
    GOLD = '\033[33m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

def run_tests():
    print(C.BOLD + "[*] Compiling cacheSim..." + C.RESET)
    compile_cmd = "g++ -std=c++11 -Wall -o cacheSim cacheSim.cpp"
    if os.system(compile_cmd) != 0:
        print(C.RED + "[!] Compilation failed!" + C.RESET)
        sys.exit(1)
    print(C.GREEN + "[OK] Compilation successful.\n" + C.RESET)

    search_pattern = os.path.join(TEST_DIR, "*.command")
    command_files = glob.glob(search_pattern)
    command_files.sort()
    
    total_tests = len(command_files)

    if not total_tests:
        print(C.RED + "[!] No tests found in folder: {}".format(TEST_DIR) + C.RESET)
        sys.exit(0)

    # Counters for summary
    passed = 0
    failed = 0
    skipped = 0

    # Enumerate gives us the index (i) starting from 1
    for i, cmd_path in enumerate(command_files, 1):
        base_path = os.path.splitext(cmd_path)[0]
        test_name = os.path.basename(base_path)
        
        input_file = "{}.in".format(base_path)
        output_file = "{}.YoursOut".format(base_path)
        ref_file = "{}.out".format(base_path)
        error_log = "{}.err".format(base_path)

        # Calculate percentage
        percent = int((float(i) / total_tests) * 100)
        prefix = "[{:3d}%]".format(percent)

        # --- ARGUMENT PARSING & SKIPPING ---
        with open(cmd_path, 'r') as f:
            content = f.read()
            tokens = content.split()

        # 1. Seek to the first real flag
        start_index = -1
        for idx, token in enumerate(tokens):
            if token.startswith("--"):
                start_index = idx
                break
        
        current_args = tokens[start_index:] if start_index != -1 else tokens

        '''
        Attention! Attention! Future people, in case the staff decides to add vic-cache tests as well, change this part, and add the skipped tests (I'm not sure how to and it's may be problematic, by not probleMYtic 
        so it's not my problem right now, it's f**king 1:20 AM and I'm tired, still need to fix my 77% pass rate, so sh...)
        '''
        # 2. Check for Victim Cache = 1
        should_skip = False
        for idx in range(len(current_args) - 1):
            if current_args[idx] == "--vic-cache" and current_args[idx+1] == "1":
                should_skip = True
                break
        
        # Print the start of the line (e.g., "[ 10%] Running test5...")
        # We use standard color for the name
        print("{} Running {}{:<15}{}...".format(prefix, C.BLUE, test_name, C.RESET), end=" ")
        sys.stdout.flush()

        if should_skip:
            print(C.YELLOW + "SKIP (Victim Cache)" + C.RESET)
            skipped += 1
            continue

        # 3. Clean Arguments
        final_args = []
        skip_next = False
        for token in current_args:
            if skip_next:
                skip_next = False
                continue
            if token == "--vic-cache":
                skip_next = True 
                continue
            final_args.append(token)

        args_str = " ".join(final_args)

        # RUN PROGRAM
        run_cmd = "./cacheSim {} {} > {} 2> {}".format(input_file, args_str, output_file, error_log)
        exit_code = os.system(run_cmd)

        # COMPARE OUTPUT
        diff_cmd = "diff -w {} {} > /dev/null".format(output_file, ref_file)
        diff_code = os.system(diff_cmd)

        # --- REPORTING ---
        if exit_code != 0:
            print(C.RED + "CRASH (Exit: {})".format(exit_code) + C.RESET)
            failed += 1
        elif os.stat(output_file).st_size == 0:
            print(C.RED + "FAIL (Empty Output)" + C.RESET)
            failed += 1
        elif diff_code != 0:
            print(C.RED + "FAIL (Mismatch)" + C.RESET)
            failed += 1
        else:
            print(C.GREEN + "PASS" + C.RESET)
            passed += 1
            if os.path.exists(error_log): os.remove(error_log)

    # --- FINAL SUMMARY ---
    print("\n" + "="*30)
    print("SUMMARY")
    print("="*30)
    print(C.GREEN + "Passed:  {}".format(passed) + C.RESET)
    print(C.RED + "Failed:  {}".format(failed) + C.RESET)
    print(C.YELLOW + "Skipped: {}".format(skipped) + C.RESET)
    print("Total tests:   {}".format(total_tests))
    print(C.FUCHSIA + "\n" + str(100 * passed / (passed + failed)) + " % passed")
    print("="*30)
    print("\n")
    if failed == 0:
        print(C.GOLD + "Wow, a 100% pass success-rate! \nYou truly are the honored one, you should go watch Jujutsu Kaisen.")
    else:
        print(C.GOLD + "You could quit here, but that`s... How losers think.")

if __name__ == "__main__":
    run_tests()
