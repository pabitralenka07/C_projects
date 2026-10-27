#!/usr/bin/env bash
# ==============================================================================
# C Language Project Portfolio — Unified Build, Test & Runner Script
# Standard: ISO/IEC 9899:2011 (C11)
# ==============================================================================

set -euo pipefail

# ANSI Color Codes
BOLD="\033[1m"
GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
CYAN="\033[1;36m"
MAGENTA="\033[1;35m"
BLUE="\033[1;34m"
DIM="\033[2m"
RESET="\033[0m"

# Project definitions: "dir:binary_name:display_name"
PROJECTS=(
    "01_hospital_management:hospital:01 Hospital Management"
    "02_bus_reservation:bus_reservation:02 Bus Reservation"
    "03_airport_management:airport:03 Airport Management"
    "04_zoo_management:zoo:04 Zoo Management"
    "05_library_management:library:05 Library Management"
    "06_bank_management:bank:06 Bank Management"
    "07_simple_calculator:calculator:07 Simple Calculator"
    "08_palindrome_checker:palindrome:08 Palindrome Checker"
    "09_factorial_calculator:factorial:09 Factorial Calculator"
    "10_todo_list:todo:10 To-Do List"
    "11_rock_paper_scissors:rps:11 Rock Paper Scissors"
    "12_currency_converter:converter:12 Currency Converter"
    "13_atm_system:atm:13 ATM System"
    "14_contact_management:contact_manager:14 Contact Management"
    "15_dictionary_app:dict:15 Dictionary App"
    "16_tic_tac_toe:tictactoe:16 Tic-Tac-Toe"
)

# Root directory of portfolio
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# Print banner
print_banner() {
    echo -e "${CYAN}${BOLD}"
    echo "======================================================================"
    echo "            C Language Production Portfolio Automation Engine          "
    echo "       Standards: C11 | ASan + UBSan | Zero Warnings | Atomic I/O      "
    echo "======================================================================"
    echo -e "${RESET}"
}

# Print usage
usage() {
    print_banner
    echo -e "${BOLD}Usage:${RESET} $0 [OPTION]"
    echo ""
    echo -e "${BOLD}Options:${RESET}"
    echo -e "  ${GREEN}(default)${RESET}          Build all projects and run complete test battery (unit + integration)"
    echo -e "  ${GREEN}--build, -b${RESET}        Compile all 16 projects without running tests"
    echo -e "  ${GREEN}--test, -t${RESET}         Execute all unit tests (with ASan+UBSan) and integration tests"
    echo -e "  ${GREEN}--unit, -u${RESET}         Run only unit test binaries"
    echo -e "  ${GREEN}--integration, -i${RESET}  Run only interactive menu integration test scripts"
    echo -e "  ${GREEN}--clean, -c${RESET}        Clean all build artifacts and temporary files across all projects"
    echo -e "  ${GREEN}--run, -r <TARGET>${RESET} Interactively launch an application (TARGET: 1-16 or project name)"
    echo -e "  ${GREEN}--demo, -d${RESET}         Execute headless smoke demos across all 16 applications"
    echo -e "  ${GREEN}--help, -h${RESET}         Show this help information"
    echo ""
    echo -e "${BOLD}Available Project Targets for --run:${RESET}"
    echo "   1 | hospital     - Hospital Management System"
    echo "   2 | bus          - Bus Reservation System"
    echo "   3 | airport      - Airport Management System"
    echo "   4 | zoo          - Zoo Management System"
    echo "   5 | library      - Library Management System"
    echo "   6 | bank         - Bank Management System"
    echo "   7 | calc         - Simple Calculator (Recursive Descent)"
    echo "   8 | palindrome   - Palindrome Checker & Substring Analyzer"
    echo "   9 | factorial    - Factorial & Combinatorics Engine"
    echo "  10 | todo         - To-Do List Manager"
    echo "  11 | rps          - Rock Paper Scissors (Best-of-N, RNG, Streak Tracker)"
    echo "  12 | converter    - Currency Converter (Multi-Currency Graph/Pivot)"
    echo "  13 | atm          - ATM System (PIN Lockout, Cent Accounting, Atomic I/O)"
    echo "  14 | contact      - Contact Management System (CSV Importer/Exporter, Atomic I/O)"
    echo "  15 | dict         - Dictionary App (Dynamic Hash Map, Quadratic Probing)"
    echo "  16 | tictactoe    - Tic-Tac-Toe (Minimax AI, Multi-Round Match Tracker)"
    echo ""
}

# Verify system prerequisites
check_prerequisites() {
    local missing=()
    command -v make >/dev/null 2>&1 || missing+=("make")
    if ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
        missing+=("gcc or clang")
    fi
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}${BOLD}Error: Missing required system dependencies:${RESET} ${missing[*]}"
        exit 1
    fi
}

# Build all projects
build_all() {
    echo -e "${BLUE}${BOLD}==> Building all 16 C Projects (Zero-Warnings Policy)...${RESET}"
    local start_time
    start_time=$(date +%s)

    make all

    local elapsed=$(($(date +%s) - start_time))
    echo -e "${GREEN}${BOLD}✓ All binaries built successfully in ${elapsed}s!${RESET}\n"
}

# Clean all projects
clean_all() {
    echo -e "${YELLOW}${BOLD}==> Cleaning build artifacts across all 16 projects...${RESET}"
    make clean
    echo -e "${GREEN}${BOLD}✓ Clean complete.${RESET}\n"
}

# Run unit tests only
run_unit_tests() {
    echo -e "${CYAN}${BOLD}==> Running Unit Tests with ASan & UBSan...${RESET}"
    local passed=0
    local total=0

    for entry in "${PROJECTS[@]}"; do
        IFS=":" read -r pdir pbin pname <<< "$entry"
        total=$((total + 1))
        echo -e "\n${BOLD}[$pname]${RESET}"
        if make -C "$pdir" test_unit; then
            passed=$((passed + 1))
            echo -e "${GREEN}✓ $pname unit tests passed.${RESET}"
        else
            echo -e "${RED}✗ $pname unit tests failed!${RESET}"
        fi
    done

    echo ""
    echo -e "${BOLD}Unit Test Summary:${RESET} ${passed}/${total} projects passed."
    if [ "$passed" -ne "$total" ]; then
        exit 1
    fi
}

# Run integration tests only
run_integration_tests() {
    echo -e "${CYAN}${BOLD}==> Running Headless Menu Integration Tests...${RESET}"
    local passed=0
    local total=0

    for entry in "${PROJECTS[@]}"; do
        IFS=":" read -r pdir pbin pname <<< "$entry"
        total=$((total + 1))
        echo -e "\n${BOLD}[$pname Integration]${RESET}"
        if make -C "$pdir" test_integration; then
            passed=$((passed + 1))
            echo -e "${GREEN}✓ $pname integration test passed.${RESET}"
        else
            echo -e "${RED}✗ $pname integration test failed!${RESET}"
        fi
    done

    echo ""
    echo -e "${BOLD}Integration Test Summary:${RESET} ${passed}/${total} projects passed."
    if [ "$passed" -ne "$total" ]; then
        exit 1
    fi
}

# Run full test suite with detailed scorecard table
run_full_tests() {
    print_banner
    build_all

    echo -e "${BLUE}${BOLD}==> Executing Comprehensive Test Battery across 16 Projects...${RESET}\n"

    local -a test_results=()
    local -a test_names=()
    local -a test_times=()
    local all_success=true
    local suite_start
    suite_start=$(date +%s)

    for entry in "${PROJECTS[@]}"; do
        IFS=":" read -r pdir pbin pname <<< "$entry"
        test_names+=("$pname")
        echo -e "${BOLD}----------------------------------------------------------------------${RESET}"
        echo -e "${MAGENTA}${BOLD}Testing: ${pname}${RESET}"
        echo -e "${BOLD}----------------------------------------------------------------------${RESET}"
        
        local p_start
        p_start=$(date +%s)
        
        if make -C "$pdir" test; then
            local p_time=$(($(date +%s) - p_start))
            test_results+=("PASS")
            test_times+=("${p_time}s")
            echo -e "${GREEN}${BOLD}✓ ${pname} PASSED in ${p_time}s${RESET}\n"
        else
            local p_time=$(($(date +%s) - p_start))
            test_results+=("FAIL")
            test_times+=("${p_time}s")
            echo -e "${RED}${BOLD}✗ ${pname} FAILED in ${p_time}s${RESET}\n"
            all_success=false
        fi
    done

    local total_elapsed=$(($(date +%s) - suite_start))

    # Print Formatted Scorecard
    echo -e "\n${CYAN}${BOLD}"
    echo "======================================================================"
    echo "                        PORTFOLIO TEST SCORECARD                      "
    echo "======================================================================"
    echo -e "${RESET}"
    printf "${BOLD}%-4s | %-32s | %-10s | %-10s${RESET}\n" "No." "Project Name" "Duration" "Status"
    echo "-----+----------------------------------+------------+------------"

    local pass_count=0
    for i in "${!test_names[@]}"; do
        local num=$((i + 1))
        local name="${test_names[$i]}"
        local dur="${test_times[$i]}"
        local res="${test_results[$i]}"
        
        if [ "$res" == "PASS" ]; then
            pass_count=$((pass_count + 1))
            printf "%-4d | %-32s | %-10s | ${GREEN}%-10s${RESET}\n" "$num" "$name" "$dur" "✓ PASS"
        else
            printf "%-4d | %-32s | %-10s | ${RED}%-10s${RESET}\n" "$num" "$name" "$dur" "✗ FAIL"
        fi
    done
    echo "-----+----------------------------------+------------+------------"
    echo -e "${BOLD}Total Duration:${RESET} ${total_elapsed}s | ${BOLD}Passed:${RESET} ${pass_count}/${#test_names[@]}"
    echo ""

    if [ "$all_success" = true ]; then
        echo -e "${GREEN}${BOLD}🎉 SUCCESS: All 16 C Projects verified with ZERO errors and ZERO sanitizer leaks!${RESET}\n"
        exit 0
    else
        echo -e "${RED}${BOLD}⚠️  FAILURE: One or more test suites failed! Check output above.${RESET}\n"
        exit 1
    fi
}

# Resolve target argument to binary path
resolve_target() {
    local target="$1"
    case "$target" in
        1|hospital|01_hospital_management)
            echo "01_hospital_management/build/hospital"
            ;;
        2|bus|02_bus_reservation)
            echo "02_bus_reservation/build/bus_reservation"
            ;;
        3|airport|03_airport_management)
            echo "03_airport_management/build/airport"
            ;;
        4|zoo|04_zoo_management)
            echo "04_zoo_management/build/zoo"
            ;;
        5|library|05_library_management)
            echo "05_library_management/build/library"
            ;;
        6|bank|06_bank_management)
            echo "06_bank_management/build/bank"
            ;;
        7|calc|calculator|07_simple_calculator)
            echo "07_simple_calculator/build/calculator"
            ;;
        8|pal|palindrome|08_palindrome_checker)
            echo "08_palindrome_checker/build/palindrome"
            ;;
        9|fact|factorial|09_factorial_calculator)
            echo "09_factorial_calculator/build/factorial"
            ;;
        10|todo|10_todo_list)
            echo "10_todo_list/build/todo"
            ;;
        11|rps|11_rock_paper_scissors)
            echo "11_rock_paper_scissors/build/rps"
            ;;
        12|converter|currency|12_currency_converter)
            echo "12_currency_converter/build/converter"
            ;;
        13|atm|13_atm_system)
            echo "13_atm_system/build/atm"
            ;;
        14|contact|14_contact_management)
            echo "14_contact_management/build/contact_manager"
            ;;
        15|dict|dictionary|15_dictionary_app)
            echo "15_dictionary_app/build/dict"
            ;;
        16|ttt|tictactoe|16_tic_tac_toe)
            echo "16_tic_tac_toe/build/tictactoe"
            ;;
        *)
            echo ""
            ;;
    esac
}

# Run a project interactively
run_project() {
    local target="$1"
    local bin_path
    bin_path=$(resolve_target "$target")

    if [ -z "$bin_path" ]; then
        echo -e "${RED}${BOLD}Error: Unknown target '$target'.${RESET}"
        echo "Use targets 1-16, or names: hospital, bus, airport, zoo, library, bank, calc, palindrome, factorial, todo, rps, converter, atm, contact, dict, tictactoe."
        exit 1
    fi

    local pdir
    pdir=$(dirname "$(dirname "$bin_path")")

    if [ ! -x "$bin_path" ]; then
        echo -e "${YELLOW}Binary '$bin_path' not found. Compiling first...${RESET}"
        make -C "$pdir"
    fi

    echo -e "${GREEN}${BOLD}==> Launching '$bin_path' (C11 Interactive Shell)...${RESET}\n"
    (cd "$pdir" && "./build/$(basename "$bin_path")")
}

# Run automated headless demo
run_demos() {
    print_banner
    build_all
    echo -e "${CYAN}${BOLD}==> Running Automated Demos across All 16 Projects...${RESET}\n"

    for entry in "${PROJECTS[@]}"; do
        IFS=":" read -r pdir pbin pname <<< "$entry"
        echo -e "${MAGENTA}${BOLD}--- Demo: $pname ---${RESET}"
        bash "$pdir/tests/test_menu.sh" "$pdir/build/$pbin"
        echo ""
    done

    echo -e "${GREEN}${BOLD}✓ All 16 automated demos executed successfully!${RESET}\n"
}

# Main Dispatcher
check_prerequisites

if [ $# -eq 0 ]; then
    run_full_tests
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --build|-b)
            print_banner
            build_all
            exit 0
            ;;
        --clean|-c)
            clean_all
            exit 0
            ;;
        --unit|-u)
            print_banner
            run_unit_tests
            exit 0
            ;;
        --integration|-i)
            print_banner
            run_integration_tests
            exit 0
            ;;
        --test|-t)
            run_full_tests
            exit 0
            ;;
        --demo|-d)
            run_demos
            exit 0
            ;;
        --run|-r)
            if [ -z "${2:-}" ]; then
                echo -e "${RED}Error: --run requires a target (1-10 or name).${RESET}"
                exit 1
            fi
            run_project "$2"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown argument: $1${RESET}"
            usage
            exit 1
            ;;
    esac
done
