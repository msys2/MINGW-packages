#!/usr/bin/awk -f
#
# masm2gas.awk - Convert MASM x64 assembly to GAS with Intel syntax.
#
# Translates MASM directives (proc/endp, SEH frame macros from
# ksamd64.inc, constant definitions) to their GAS equivalents while
# passing instructions through unchanged.  The output file uses
# .intel_syntax noprefix so that Intel-format instructions do not
# need operand-order or register-prefix conversion.
#
# Usage:
#   awk -f masm2gas.awk input.asm > output.S

BEGIN {
    print ".intel_syntax noprefix"
    print ""
    in_frame = 0
}

# Strip trailing CR (Windows line endings)
{ sub(/\r$/, "") }

# Strip trailing whitespace
{ sub(/[[:space:]]+$/, "") }

# --- Skip MASM-only directives (matched before comment conversion) ---

/^[[:space:]]*\.code/  { print ".text"; next }
/^[[:space:]]*\.data/  { print ".data"; next }

# --- Data definitions:  label db/dw/dd/dq value ---

/^[A-Za-z_][A-Za-z_0-9]*[[:space:]]+(db|dw|dd|dq)[[:space:]]/ {
    label = $1
    size = $2
    val = $3
    if (size == "db") gas = ".byte"
    else if (size == "dw") gas = ".short"
    else if (size == "dd") gas = ".long"
    else if (size == "dq") gas = ".quad"
    data_sym[label] = 1
    printf "%s:\n\t%s %s\n", label, gas, val
    next
}
/^[[:space:]]*include[[:space:]]+ksamd64\.inc/ { next }
/^[[:space:]]*end[[:space:]]*$/   { next }
/^[[:space:]]*end[[:space:]]*;/   { next }
/^[[:space:]]*option[[:space:]]+prologue:none/ { next }

# --- Constant definition:  NAME = EXPR ---

/^[A-Za-z_][A-Za-z_0-9]*[[:space:]]*=[[:space:]]/ {
    name = $1
    line = $0
    sub(/^[^=]*=[[:space:]]*/, "", line)
    sub(/[[:space:]]*;.*/, "", line)
    printf ".equ %s, %s\n", name, line
    next
}

# --- FUNCNAME proc frame ---

/^[A-Za-z_][A-Za-z_0-9]*[[:space:]]+proc[[:space:]]+frame/ {
    current_func = $1
    in_frame = 1
    print ""
    printf ".globl %s\n", current_func
    printf ".def %s; .scl 2; .type 32; .endef\n", current_func
    printf "%s:\n", current_func
    printf "\t.seh_proc %s\n", current_func
    next
}

# --- FUNCNAME proc  (no frame) ---

/^[A-Za-z_][A-Za-z_0-9]*[[:space:]]+proc[[:space:]]*$/ ||
/^[A-Za-z_][A-Za-z_0-9]*[[:space:]]+proc[[:space:]]*;/ {
    current_func = $1
    in_frame = 0
    print ""
    printf ".globl %s\n", current_func
    printf ".def %s; .scl 2; .type 32; .endef\n", current_func
    printf "%s:\n", current_func
    next
}

# --- FUNCNAME endp ---

/[[:space:]]+endp/ {
    if (in_frame) print "\t.seh_endproc"
    in_frame = 0
    print ""
    next
}

# --- ksamd64.inc SEH macros ---

/^[[:space:]]*push_reg[[:space:]]/ {
    printf "\tpush %s\n", $2
    printf "\t.seh_pushreg %s\n", $2
    next
}

/^[[:space:]]*set_frame[[:space:]]/ {
    reg = $2; sub(/,/, "", reg)
    offset = $3 + 0
    if (offset == 0)
        printf "\tmov %s, rsp\n", reg
    else
        printf "\tlea %s, [rsp+%d]\n", reg, offset
    printf "\t.seh_setframe %s, %d\n", reg, offset
    next
}

/^[[:space:]]*\.endprolog/ {
    print "\t.seh_endprologue"
    next
}

/^[[:space:]]*\.allocstack[[:space:]]/ {
    size_expr = $0
    sub(/^[[:space:]]*\.allocstack[[:space:]]+/, "", size_expr)
    sub(/[[:space:]]*;.*/, "", size_expr)
    printf "\t.seh_stackalloc %s\n", size_expr
    next
}

# --- Default: fix data-symbol addressing, convert comments, pass through ---
# MASM x64 uses RIP-relative addressing implicitly for data symbols;
# GAS Intel syntax requires the explicit [rip + sym] form.

{
    for (sym in data_sym) {
        gsub("\\[" sym "\\]", "[rip + " sym "]")
        gsub("\\[" sym " \\+", "[rip + " sym " +")
        gsub("\\[" sym " \\-", "[rip + " sym " -")
    }
    gsub(/;/, "//")
    print
}
