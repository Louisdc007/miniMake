#!/bin/bash

YAML_FILE="tests.yaml"
# Ensure we append to a clean state or just append
echo "" >> "$YAML_FILE"
echo "  # ══════════════════════════════════════════════════════════" >> "$YAML_FILE"
echo "  #  TESTS AVANCES (exec_global & exec_variables)" >> "$YAML_FILE"
echo "  # ══════════════════════════════════════════════════════════" >> "$YAML_FILE"

TEST_COUNTER=1

add_test() {
    local name="$1"
    local category="$2"
    shift 2
    local content="$1"
    
    local in_file="test_adv_${TEST_COUNTER}.in"
    local out_file="test_adv_${TEST_COUNTER}.out_e"
    
    # Create Makefile using printf %b to evaluate \n and \t properly
    printf "%b\n" "$content" > "$in_file"
    
    # Run GNU make to get ground truth
    make -f "$in_file" > "$out_file" 2>&1
    local code=$?
    
    # Replace 'make:' with 'minimake:' in the ground truth output
    sed -i '' 's/^make: /minimake: /g' "$out_file"
    sed -i '' 's/make\[[0-9]*\]: /minimake: /g' "$out_file"
    
    # Standardize backticks to single quotes (GNU make uses \`...\' sometimes)
    sed -i '' "s/\`/'/g" "$out_file"
    
    # Append to yaml
    cat << EOF >> "$YAML_FILE"
  - name: "$name"
    category: "$category"
    args: ["-f", "tests/$in_file"]
    exit_code: $code
    output_file: "tests/$out_file"

EOF
    
    TEST_COUNTER=$((TEST_COUNTER + 1))
}

# ---------------------------------------------------------
# Variables Expansion Tests (exec_variables)
# ---------------------------------------------------------

# 1-10: Basic Syntax
add_test "Var syntax \$(VAR)" "exec_variables" "A=hello\nall:\n\techo \$(A)"
add_test "Var syntax \${VAR}" "exec_variables" "A=hello\nall:\n\techo \${A}"
add_test "Var syntax \$A" "exec_variables" "A=hello\nall:\n\techo \$A"
add_test "Var syntax \$\$" "exec_variables" "all:\n\techo \$\$"
add_test "Var syntax \$\$\$A" "exec_variables" "A=hello\nall:\n\techo \$\$\$A"
add_test "Var unknown" "exec_variables" "all:\n\techo \$(UNKNOWN)"
add_test "Var empty" "exec_variables" "A=\nall:\n\techo \$(A)world"
add_test "Var spaces" "exec_variables" "A=   hello   \nall:\n\techo \$(A)world"
add_test "Var numbers" "exec_variables" "A1=hello\nall:\n\techo \$(A1)"
add_test "Var special chars" "exec_variables" "A_B=hello\nall:\n\techo \$(A_B)"

# 11-20: Recursive / Nested Expansion
add_test "Nested var 1" "exec_variables" "A=hello\nB=\$(A) world\nall:\n\techo \$(B)"
add_test "Nested var 2" "exec_variables" "A=\$(B)\nB=hello\nall:\n\techo \$(A)"
add_test "Nested var 3" "exec_variables" "A=\$(B) \$(C)\nB=hello\nC=world\nall:\n\techo \$(A)"
add_test "Nested var 4" "exec_variables" "A=\$(B)\nB=\$(C)\nC=hello\nall:\n\techo \$(A)"
add_test "Nested var 5" "exec_variables" "A=\$(B)\nB=hello \$(C)\nC=world\nall:\n\techo \$(A)"
add_test "Nested var missing" "exec_variables" "A=\$(B)\nall:\n\techo \$(A)"
add_test "Var in target" "exec_variables" "T=all\n\$(T):\n\techo hello"
add_test "Var in dep" "exec_variables" "D=dep\nall: \$(D)\ndep:\n\techo dep"
add_test "Var in recipe" "exec_variables" "CMD=echo\nall:\n\t\$(CMD) hello"
add_test "Var in target and dep" "exec_variables" "T=all\nD=dep\n\$(T): \$(D)\ndep:\n\techo dep"

# 21-30: Automatic Variables
add_test "Auto \$@" "exec_variables" "all:\n\techo \$@"
add_test "Auto \$<" "exec_variables" "all: dep1 dep2\n\techo \$<\ndep1:\ndep2:"
add_test "Auto \$^" "exec_variables" "all: dep1 dep2\n\techo \$^\ndep1:\ndep2:"
add_test "Auto \$@ nested" "exec_variables" "A=\$@\nall:\n\techo \$(A)"
add_test "Auto \$< nested" "exec_variables" "A=\|<\nall: dep1\n\techo \$(A)\ndep1:"
add_test "Auto \$^ nested" "exec_variables" "A=\$^\nall: dep1 dep2\n\techo \$(A)\ndep1:\ndep2:"
add_test "Auto variables combined" "exec_variables" "all: dep1 dep2\n\techo \$@ \$< \$^\ndep1:\ndep2:"
add_test "Auto variables with suffix" "exec_variables" "all: dep1\n\techo \$@.txt \$<.c\ndep1:"
add_test "Auto variables empty dep" "exec_variables" "all:\n\techo \$<"
add_test "Auto variables \$@" "exec_variables" "target_with_long_name:\n\techo \$@"

# ---------------------------------------------------------
# Global Execution Tests (exec_global)
# ---------------------------------------------------------

# 31-40: Rule parsing & Dependencies
add_test "No deps" "exec_global" "all:\n\techo hello"
add_test "One dep" "exec_global" "all: dep\n\techo all\ndep:\n\techo dep"
add_test "Multiple deps" "exec_global" "all: d1 d2 d3\n\techo all\nd1:\n\techo d1\nd2:\n\techo d2\nd3:\n\techo d3"
add_test "Deep deps" "exec_global" "all: d1\n\techo all\nd1: d2\n\techo d1\nd2: d3\n\techo d2\nd3:\n\techo d3"
add_test "Repeated deps" "exec_global" "all: d1 d1 d1\n\techo all\nd1:\n\techo d1"
add_test "Circular deps fallback" "exec_global" "all: d1\nd1: d2\nd2: d1"
add_test "Missing dep" "exec_global" "all: missing\n\techo all"
add_test "Missing target" "exec_global" "all: d1\nd1:\n\techo d1"
add_test "Empty line between rules" "exec_global" "all:\n\techo all\n\n\nd1:\n\techo d1"
add_test "Comments everywhere" "exec_global" "# comment\nall: # comment\n\techo all # comment\n# comment"

# 41-50: Recipes & Commands
add_test "Multiple recipes" "exec_global" "all:\n\techo 1\n\techo 2\n\techo 3"
add_test "Failing recipe" "exec_global" "all:\n\tfalse\n\techo not reached"
add_test "Empty recipe line" "exec_global" "all:\n\t\n\techo hello"
add_test "Spaces before target" "exec_global" "all :\n\techo hello"
add_test "Spaces in deps" "exec_global" "all:   d1    d2\n\techo all\nd1:\nd2:"
add_test "Target with dot" "exec_global" "all.txt:\n\techo all"
add_test "Target with underscore" "exec_global" "my_target:\n\techo hello"
add_test "Target with dash" "exec_global" "my-target:\n\techo hello"
add_test "Recipe with exit" "exec_global" "all:\n\tsh -c 'exit 2'"
add_test "Many rules" "exec_global" "all: a b c d e f g h i j\na:\nb:\nc:\nd:\ne:\nf:\ng:\nh:\ni:\nj:"

echo "50 advanced tests generated and appended to $YAML_FILE"
