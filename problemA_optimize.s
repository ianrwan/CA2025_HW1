.data

input_text: .string "Input:"
encode_text: .string "Encode:"
decode_text: .string "Decode:"
new_line: .string "\n"

uf8_lower_bound:
    .word 0        # e=0
    .word 16       # e=1
    .word 48       # e=2
    .word 112      # e=3
    .word 240      # e=4
    .word 496      # e=5
    .word 1008     # e=6
    .word 2032     # e=7
    .word 4080     # e=8
    .word 8176     # e=9
    .word 16368    # e=10
    .word 32752    # e=11
    .word 65520    # e=12
    .word 131056   # e=13
    .word 262128   # e=14
    .word 524272   # e=15

.text
main:
    li s11, 0
    li a0, 0x64 # test data
test: 
    jal ra, input_text_console
 
    mv s0, a0
    jal ra, uf8_decode
    jal ra, decode_text_console
    
    mv s1, a0
    jal ra, uf8_encode_optimized
    jal ra, encode_text_console
    
    li t0, 3
    addi s11, s11, 1
    mv a0, s0
    addi a0, a0, 47
    ble s11, t0, test
    jal zero, end
    

# clz function
# a0 = x
clz:
    li t0, 32 # t0 = n = 32
    li t1, 16 # t1 = c = 16
    
clz_while:
    srl t2, a0, t1 # t2 = y = x >> c
    beq t2, zero, clz_y_false # if y == 0 jump
    sub t0, t0, t1 # t0 = n = n-c
    mv a0, t2 # a0 = x = y

clz_y_false:
    srli t1, t1, 1 # t1 = c = c >> 1
    bne t1, zero, clz_while # if t1 != 0 continue
    sub a0, t0, a0 # a0 = n-x
    jalr ra

# uf8_decode function
# a0 = f1

uf8_decode:
    addi sp, sp, -4
    sw ra, 0(sp)
    
    andi t0, a0, 0x0f # t0 = mantissa = f1 & 0x0f
    srli t1, a0, 4 # t1 = exponent = f1 >> 4
    
    li t2, 15
    sub t2, t2, t1 # t2 = 15 - exponent
    li t3, 0x7fff
    srl t2, t3, t2 # t2 = 0x7fff >> (15-exponent)
    slli t2, t2, 4 # t2 = offset = 0x7fff >> (15-exponent) << 4
    
    sll t0, t0, t1 # t0 = mantissa << exponent
    add a0, t0, t2 # a0 = (mantissa << exponent) + offset
    
    lw ra, 0(sp)
    addi sp, sp, 4
    jalr ra

# uf8_encode_optimized_function
# a0 = value
uf8_encode_optimized:
    addi sp, sp, -20
    sw ra, 0(sp)
    sw s0, 4(sp)
    sw s1, 8(sp)
    sw s2, 12(sp)
    sw s3, 16(sp)
    
    mv s0, a0 # s0 = value
    
    li t0, 16 # t0 = 16
    bge s0, t0, uf8_encode_optimized_value_bge_16 # if value >= 16 jump 
    
    jal zero, uf8_encode_optimized_return # return value (a0 is still value)
    
uf8_encode_optimized_value_bge_16:
    li s1, 1 # s1 = low = 1
    li s2, 15 # s2 = high = 15
    li s3, 0 # s3 = exponent = 0
    
uf8_encode_optimized_while:
    addi t0, s2, 1 # t0 = high + 1
    bge s1, t0, uf8_encode_optimized_end_while # if low >= high + 1 jump
    
    add t1, s1, s2 # t1 = (low+high)
    srli t1, t1 , 1 #　t1 = mid = (low+high) / 2
    
    la t2, uf8_lower_bound # t2 = uf8_lower_bound
    slli t3, t1, 2 # t3 = t1 * 4
    add t2, t2 ,t3 # t2 = uf8_lower_bound + mid
    lw t2, 0(t2) # t2 = uf8_lower_bound[mid]
    
    ble s0, t2, uf8_encode_optimized_else # if value < uf8_lower_bound[mid]
    mv s3, t1 # s3 = exponent = mid
    addi s1, t1, 1 # s1 = low = mid+1
    jal zero, uf8_encode_optimized_while_continue # end if
    
uf8_encode_optimized_else:
    addi s2, t1, -1 # s2 = high = mid-1
    
uf8_encode_optimized_while_continue:
    jal zero, uf8_encode_optimized_while # while
    
uf8_encode_optimized_end_while:
    la t0, uf8_lower_bound # t0 = uf8_lower_bound
    slli t4, s3, 2 # t4 = exponent * 4 (translate to bite address)
    add t1, t0 , t4 # t1 = uf8_lower_bound + exponent
    lw t2, 0(t1) # t2 = offset = uf8_lower_bound[exponent]
    
    sub s0, s0, t2 # s0 = value - offset
    srl s0, s0, s3 # s0 = mantissa = (value - offset) >> exponent
    
    slli s3, s3, 4 # s3 = exponent << 4
    
    or a0, s3, s0 # a0 = (exponent << 4) | mantissa
    
uf8_encode_optimized_return:
    lw ra, 0(sp)
    lw s0, 4(sp)
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    addi sp, sp, 20
    jalr ra
    
input_text_console:
    mv t0, a0
    
    la a0, input_text
    li a7, 4
    ecall
    
    mv a0, t0
    li a7, 1
    ecall
    
    la a0, new_line
    li a7, 4
    ecall
    
    mv a0, t0
    jalr ra
    
encode_text_console:
    mv t0, a0
    la a0, encode_text
    li a7, 4
    ecall
    
    mv a0, t0
    li a7, 1
    ecall
    
    la a0, new_line
    li a7, 4
    ecall
    
    la a0, new_line
    li a7, 4
    ecall
    
    mv a0, t0
    jalr ra
    
decode_text_console:
    mv t0, a0
    la a0, decode_text
    li a7, 4
    ecall
    
    mv a0, t0
    li a7, 1
    ecall
    
    la a0, new_line
    li a7, 4
    ecall
    
    mv a0, t0
    jalr ra

end:
    add zero, zero, zero
