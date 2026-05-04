#!/usr/bin/env python3
"""
testsuite.py — Suite de tests exhaustive pour minimake.

Fonctionnalités du harness :
  - check_stdout   : bool (défaut True)  — compare stdout avec output_file
  - check_exit     : bool (défaut True)  — vérifie le code de retour
  - exit_code      : int  (défaut 0)
  - output_file    : str  — fichier contenant le stdout attendu
  - setup          : list[str] — commandes shell lancées AVANT le test (cwd=ROOT_DIR)
  - teardown       : list[str] — commandes shell lancées APRÈS le test (toujours)
"""

import subprocess
import pathlib
import sys
import yaml

# ---------------------------------------------------------------------------
# Chemins
# ---------------------------------------------------------------------------
ROOT_DIR     = pathlib.Path(__file__).resolve().parent.parent
MINIMAKE_BIN = ROOT_DIR / "minimake"
YAML_FILE    = ROOT_DIR / "tests" / "tests.yaml"

# ---------------------------------------------------------------------------
# Couleurs ANSI
# ---------------------------------------------------------------------------
GREEN  = "\033[92m"
RED    = "\033[91m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
BOLD   = "\033[1m"
RESET  = "\033[0m"

def ok(name):
    print(f"  {GREEN}[PASS]{RESET} {name}")

def fail(name, reason):
    print(f"  {RED}[FAIL]{RESET} {BOLD}{name}{RESET}")
    for line in reason.splitlines():
        print(f"         {line}")

def section(title):
    print(f"\n{CYAN}{BOLD}── {title} {'─' * max(0, 54 - len(title))}{RESET}")

# ---------------------------------------------------------------------------
# Utilitaires
# ---------------------------------------------------------------------------
def run_shell(cmd, cwd):
    """Lance une commande shell (pour setup/teardown). Ignore les erreurs."""
    subprocess.run(cmd, shell=True, cwd=str(cwd),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def repr_text(text, max_lines=20):
    lines = text.splitlines(keepends=True)
    snippet = "".join(lines[:max_lines])
    if len(lines) > max_lines:
        snippet += f"... ({len(lines) - max_lines} more lines)\n"
    return snippet

def diff_summary(expected, got):
    exp_lines = expected.splitlines(keepends=True)
    got_lines = got.splitlines(keepends=True)
    lines = []
    for i, (e, g) in enumerate(zip(exp_lines, got_lines), 1):
        if e != g:
            lines.append(f"  line {i}: expected {e.rstrip()!r}  got {g.rstrip()!r}")
    if len(exp_lines) != len(got_lines):
        lines.append(f"  expected {len(exp_lines)} lines, got {len(got_lines)} lines")
    return "\n".join(lines) if lines else "(no line diff found)"

# ---------------------------------------------------------------------------
# Exécution d'un test
# ---------------------------------------------------------------------------
def run_test(test):
    name          = test.get("name", str(test.get("args", [])))
    args          = test.get("args", [])
    expected_exit = test.get("exit_code", 0)
    output_file   = test.get("output_file")
    check_stdout  = test.get("check_stdout", True)
    check_exit    = test.get("check_exit", True)
    setup_cmds    = test.get("setup", [])
    teardown_cmds = test.get("teardown", [])

    # --- Setup ---
    for cmd in setup_cmds:
        run_shell(cmd, ROOT_DIR)

    success = True
    reasons = []

    try:
        result = subprocess.run(
            [str(MINIMAKE_BIN)] + [str(a) for a in args],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=str(ROOT_DIR)
        )

        # 1. Code de retour
        if check_exit and result.returncode != expected_exit:
            success = False
            reasons.append(
                f"exit code: expected {expected_exit}, got {result.returncode}\n"
                f"  stderr: {result.stderr.strip()!r}"
            )

        # 2. Stdout
        if check_stdout:
            if not output_file:
                success = False
                reasons.append("check_stdout=True but no output_file specified")
            else:
                expected_path = ROOT_DIR / output_file
                if not expected_path.exists():
                    success = False
                    reasons.append(f"output file not found: {expected_path}")
                else:
                    expected_output = expected_path.read_text()
                    if result.stdout != expected_output:
                        success = False
                        reasons.append(
                            f"stdout mismatch:\n"
                            f"  {diff_summary(expected_output, result.stdout)}\n"
                            f"  ---- expected ----\n"
                            f"{repr_text(expected_output)}"
                            f"  ---- got --------\n"
                            f"{repr_text(result.stdout)}"
                        )

    except Exception as e:
        success = False
        reasons.append(f"exception: {e}")

    finally:
        # --- Teardown (toujours exécuté) ---
        for cmd in teardown_cmds:
            run_shell(cmd, ROOT_DIR)

    if success:
        ok(name)
    else:
        fail(name, "\n".join(reasons))

    return success

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    # Pré-requis
    if not MINIMAKE_BIN.exists():
        print(f"{RED}[CRITICAL]{RESET} binary not found: {MINIMAKE_BIN}")
        sys.exit(1)
    if not YAML_FILE.exists():
        print(f"{RED}[CRITICAL]{RESET} tests.yaml not found: {YAML_FILE}")
        sys.exit(1)

    with open(YAML_FILE) as f:
        try:
            data = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            print(f"{RED}[CRITICAL]{RESET} YAML error: {exc}")
            sys.exit(1)

    all_tests = data.get("tests", [])
    if not all_tests:
        print("No tests found.")
        sys.exit(1)

    # Regroupement par catégorie (champ optionnel "category")
    categories = {}
    for t in all_tests:
        cat = t.get("category", "Uncategorised")
        categories.setdefault(cat, []).append(t)

    total   = 0
    passed  = 0
    failed_names = []

    for cat, tests in categories.items():
        section(cat)
        for t in tests:
            total += 1
            if run_test(t):
                passed += 1
            else:
                failed_names.append(t.get("name", "?"))

    # Bilan
    print(f"\n{BOLD}{'─'*60}{RESET}")
    color = GREEN if passed == total else RED
    print(f"{color}{BOLD}  {passed}/{total} tests passed{RESET}")
    if failed_names:
        print(f"  {RED}Failed:{RESET}")
        for n in failed_names:
            print(f"    • {n}")
        print()
        sys.exit(1)
    else:
        print(f"  {GREEN}ALL TESTS PASSED ✓{RESET}\n")
        sys.exit(0)

if __name__ == "__main__":
    main()