
############## leipan ################

############################ DO NOT CREATE A .data SECTION ############################
############################ DO NOT CREATE A .data SECTION ############################
############################ DO NOT CREATE A .data SECTION ############################
.text:
.globl create_person
create_person:
  addi $sp,$sp, -4 #allocate space in stack 
  sw $a0, 0($sp) #stores orginal a0 address

  lbu $t0, 0($a0) #t0=max # of nodes
  lbu $t1, 16($a0) #t0= curr # of nodes
  beq	$t0, $t1, create_person_fail # if no more space, creation failed

  lw $t2, 8($a0) #t2=size of node
  mult $t2, $t1 
  mflo $t0	#$t0=free node
  
  addi $t0, $t0, 36 #$t0=free node
  add	$a0, $a0, $t0 #$a0=free node
  move $v0, $a0 #v0=free node address

  li $t1, 0 #t0=0
  initialize_node:
    sb $t1, 0($a0) #a[i]=0
    addi $t2, $t2, -1 #t2--
    addi $a0, $a0, 1 #network++
    bnez $t2, initialize_node
  
  lw $a0, 0($sp) #reloads $a0 as original address
  addi $a0,$a0, 16 #a0 -> curr_num_of_nodes
  lw $t0, 0($a0) #t0=# of nodes
  addi $t0, $t0, 1 #t0++
  sw $t0, 0($a0) #curr_num_of_node++

  addi $sp, $sp, 4 #deallocate stack
  jr $ra
     
  create_person_fail:
    addi $sp, $sp, 4 #deallocate stack
    li $v0, -1 #$v0=-1
    jr $ra





.globl is_person_exists
is_person_exists:
  
  lw $t1 8($a0) #t1=size of node
  lw $t0 16($a0) #t0=curr num of nodes

  mult	$t0, $t1
  mflo	$t0	#t0= node bound 

  addi $a0, $a0, 36 #a0 -> set of nodes

  li $t2, 0 #t2=counter
  check_is_person_loop:
    bge	$t2, $t0, is_person_exists_fail	#did not find person
    beq $a0, $a1, is_person_exists_success #matched address 
    add $a0, $a0, $t1 #node++
    add $t2, $t2, $t1 #counter++
    j check_is_person_loop
   
  is_person_exists_success:
    li $v0, 1
    jr $ra

  is_person_exists_fail:
    li $v0, 0
    jr $ra




.globl is_person_name_exists
is_person_name_exists:
  addi $sp, $sp, -16 #allocate stack space   
  sw $ra, 0($sp) #stores original ra 
  sw $a1, 12($sp) #stores the original address of name char 

  lw $t0, 16($a0) #t0= curr num of nodes
  lw $t1, 8($a0) #t1=size of node 
  sw $t1, 4($sp) #stores the size of each node

  mult $t0, $t1
  mflo $t2 #t0=node bound 

  addi $a0, $a0, 36 #a0 -> set of nodes 

  li $t3, 0 #t3=counter
  check_person_name_loop:
    beq $t3, $t2, is_person_name_exists_fail #did not find name
    
    sw $a0, 8($sp) #stores current node address
    compare_char:
      lbu $t4, 0($a0) #t4=node name[i]
      lbu $t5, 0($a1) #t5=name[i]
      beq $t4, $0, end_of_node #end of node char
      bne $t4, $t5, post_compare_char #chars are not equal to each other 
      addi $a0, $a0, 1 #node name[i++] 
      addi $a1, $a1, 1 #name [i++]
      j compare_char
    
    end_of_node:
      beq $t5, $0, is_person_name_exists_success
      j post_compare_char

    post_compare_char:
      lw $t1, 4($sp) #restores the size of each node 
      lw $a0, 8($sp) #restores the current address of $a0 
      lw $a1, 12($sp) #restores the starting address of name 
      add $a0, $a0, $t1 #node++
      add $t3, $t3, $t1 #counter++
      j check_person_name_loop

  is_person_name_exists_fail:
    lw $ra, 0($sp) #reloads original $ra
    addi $sp, $sp, 16 #deallocate stack space 
    li $v0, 0
    jr $ra

  is_person_name_exists_success:
    lw $a0, 8($sp) #restores the current address of $a0
    move $v1, $a0
    lw $ra, 0($sp) #reloads original $ra
    addi $sp, $sp, 16 #deallocate stack space 
    li $v0, 1
    jr $ra 





.globl add_person_property
add_person_property:
  addi $sp $sp, -16 #allocate space in stack 
  sw $ra 0($sp) #stores the original $ra value
  sw $a0, 4($sp) #stores the original network address
  sw $a1, 8($sp) #stores the original address of the person
  sw $a3, 12($sp) #stores the prop_val

  addi $a0, $a0, 24 #network->"NAME"
  check_for_prop_name:
    lbu $t0, 0($a0) #t0=network name property[i]
    lbu $t1, 0($a2) #t0=prop_name[i]
    beq $t0, $0, post_check_for_prop_name
    bne $t0, $t1, prop_name_error #not "NAME"
    addi $a0, $a0, 1 #network name prop[i++]
    addi $a2, $a2, 1 #prop_name[i++]
    j check_for_prop_name
  
  post_check_for_prop_name:
    bne $t0, $t0, prop_name_error
    j check_for_person_exist

  check_for_person_exist:
    lw $a0, 4($sp) #restores the original network address
    jal is_person_exists
    beqz $v0, person_error
    j check_num_of_char


  check_num_of_char:
    lw $a0, 4($sp) #resores the original network address
    lw $t0 8($a0) #loads the max size of node
    
    li $t2, 0 #t2=counter
    count_num_of_char_loop:
      lbu $t1, 0($a3) #t1=prop_val[i]
      beq $t1, $0, post_count_num_of_char #reached the end of the string
      addi $a3, $a3, 1 #prop_val[i++]
      addi $t2, $t2, 1 #counter++
      j count_num_of_char_loop 

    post_count_num_of_char:
      bge $t2, $t0, size_error #if prop_val is >= to size of node 
      j check_is_unique
  
  check_is_unique:
    lw $a3, 12($sp) #restores the orgial address of prop_val
    move $a1, $a3 #a1=a3
    jal is_person_name_exists
    bnez $v0, prop_val_error
    j update_network

  update_network:
    lw $a1, 8($sp) #restores the original address of the person
    lw $a3, 12($sp) #retores the original address of prop_val
    move $a0, $a1 #a0=address of person 

    copy_char_loop:
      lbu $t0, 0($a3), #t0=prop_val[i]
      beq $t0, $0, add_person_property_success
      sb $t0 0($a0) #stores prop_val[i] in node[i]
      addi $a3, $a3, 1 #prop_val[i++]
      addi $a0, $a0, 1 #node[i++]
      j copy_char_loop


  add_person_property_success:
    li $v0, 1
    j add_person_property_done
  
  prop_name_error:
    li $v0, 0
    j add_person_property_done
  
  person_error:
    li $v0, -1
    j add_person_property_done

  size_error:
    li $v0, -2 
    j add_person_property_done

  prop_val_error:
    li $v0, -3
    j add_person_property_done
  

  add_person_property_done:
    lw $ra 0($sp) #restores the original $ra value 
    addi $sp, $sp, 16 #deallocate stack
    jr $ra 





.globl get_person
get_person:
  addi $sp, $sp, -16 #allocate stack space 
  sw $ra, 0($sp) #stores the original address of ra 
  sw $a0 4($sp) #stores the original network address
  sw $a1 8($sp) #stores the name char
  
  jal is_person_name_exists
  beqz $v0, get_person_fail

  lw $a0 4($sp) #restores the original network address
  lw $a1 8($sp) #restores the name char

  lw $t0, 12($a0) #t0=size of node 

  addi $a0, $a0, 36 #a0->nodes
  
  find_node_address_loop:
    sw $a0, 12($sp) #stores the current node address
    find_node_compare_char:
      lbu $t1, 0($a0) #t1=node name[i]
      lbu $t2, 0($a1) #t2=name[i]
      beq $t1, $0, end_of_find_node #end of node char
      bne $t1, $t2, post_find_node_compare_char #chars are not equal
      addi $a0, $a0, 1 #node name[i++]
      addi $a1, $a1, 1 #name[i++]
      j find_node_compare_char
    
    end_of_find_node:
      beq $t2, $0, get_person_success
      j post_find_node_compare_char
    
    post_find_node_compare_char:
      lw $a0 12($sp) #restores the current address of $a0
      lw $a1, 8($sp) #restores the name char address
      add $a0, $a0, $t0 #node++
      j find_node_address_loop

  
  get_person_fail:
    li $v0, 0
    j get_person_done

  get_person_success:
    lw $a0, 12($sp) #restores the current node address
    move $v0, $a0 #v0=node address
    j get_person_done
  
  get_person_done:
    lw $ra 0($sp) #restores the original address of ra
    addi $sp, $sp, 16 #deallocate stack space 
    jr $ra 



.globl is_relation_exists
is_relation_exists:
  addi $sp, $sp, -4 # allocate stack space 
  lbu $t0, 20($a0) #t0=current num of edges
  lbu $t1, 12($a0) #t1=size of edges 

  mult $t0, $t1		
  mflo $t0	#$t0= edge bound 


  lbu $t2, 0($a0)#t0=total # of nodes
  lbu $t3, 8($a0)#t1=size of node
  mult $t2, $t3 
  mflo $t2, #t0=total bytes for node

  addi $t2, $t2, 36 #t0=total bytes for nodes+etc
  add $a0, $a0, $t2 #$a0 -> set of edges 
  
  is_relation_exists_loop:
    beqz $t0, is_relation_exits_not_found
    sw $a0, 0($sp) #stores current edge address 
      
      check_p1:
        lw $t2, 0($a0) #t2=address of p1
        beq $a1, $t2, check_a2
        beq $a2, $t2, check_a1
        j post_check_p1

      check_a2:
        lw $t2, 4($a0) #t2= address of p2
        beq $a2, $t2, is_relation_exists_found #found match
        j post_check_p1

      check_a1:
        lw $t2, 4($a0) #t2=addres of p2
        beq $a1, $t2, is_relation_exists_found #found match 
        j post_check_p1

    post_check_p1:
      lw $a0, 0($sp) #restores current edge address
      add $a0, $a0, $t1 #edge[i++]
      sub $t0, $t0, $t1 #counter--
      j is_relation_exists_loop


  is_relation_exists_found:
    li $v0, 1
    j is_relation_exists_done

  is_relation_exits_not_found:
    li $v0, 0
    j is_relation_exists_done
  
  is_relation_exists_done:
    addi $sp, $sp, 4 #deallocate stack space 
    jr $ra




.globl add_relation
add_relation:
  addi $sp, $sp, -12 #allocate space in stack pointer
  sw $ra, 0($sp) #stores the original ra value 
  sw $a1, 4($sp) #stores the original p1 
  sw $a0, 8($sp) #store the original network address

  is_both_person_in_network:
    jal is_person_exists
    beqz $v0, someone_does_not_exist
    lw $a0, 8($sp) #restores the original network address 
    move $a1, $a2 #a1=p2
    jal is_person_exists
    beqz $v0, someone_does_not_exist
    lw $a1, 4($sp) #restores the original p1
  
  check_for_capacity:
    lw $a0, 8($sp) #restores the original network address
    lw $t0, 4($a0) #t0= maximum number of edges
    lw $t1, 20($a0) #t1= curr #number of edges
    beq $t0, $t1, at_max_capacity

  check_if_unique_relationship:
    jal is_relation_exists
    bnez $v0, unique_relationship_fail
  
  check_if_related_to_self:
    beq $a1, $a2, related_to_self_fail
  
  add_relation_to_edge:
    lw $a0, 8($sp) #restores the original network address
    lw $t0, 0($a0) #t0=total nodes
    lw $t1, 8($a0) #t1=size of nodes
    lw $t2, 12($a0) #t2=size of edge
    lw $t3, 20($a0) #t3= current number of edges 

    mult	$t0, $t1
    mflo	$t0 #t0=node bound

    addi $t0, $t0, 36 #t0=start of edge address
    add $a0, $a0, $t0 #a0 -> edges

    mult $t2, $t3
    mflo $t0 #t0=next available edge 
    add $a0, $a0, $t0 #a0= address of available edge 

    sw $a1, 0($a0) #stores p1 in edge
    sw $a2, 4($a0) #stores p2 in edge

    lw $a0, 8($sp) #restores  the original network address
    addi $a0, $a0, 20 #a0->current number of edges
    lw $t0, 0($a0) #t0=curr # of edges
    addi $t0, $t0, 1 #edges+1
    sw $t0, 0($a0) #update edges

    li $v0, 1
    j add_relation_done

  someone_does_not_exist:
    li $v0, 0
    j add_relation_done

  at_max_capacity:
    li $v0, -1
    j add_relation_done

  unique_relationship_fail:
    li $v0, -2
    j add_relation_done
  
  related_to_self_fail:
    li $v0, -3
    j add_relation_done

  add_relation_done:
    lw $ra, 0($sp) #restores the original ra
    addi $sp, $sp, 12 #deallocate stack pointer 
    jr $ra



.globl add_relation_property
add_relation_property:
  addi $sp, $sp, -20 #allocate space in stack pointer
  sw $ra, 0($sp) #stores the original ra in sp
  sw $a0, 4($sp) #stores the original a0 address
  sw $a1, 8($sp) #stores the original a1
  sw $a2, 12($sp) #stores the original a2

  check_if_relation_exist:
    jal is_relation_exists
    beqz $v0, relation_does_not_exist
    j check_prop_name

  check_prop_name:
    lw $a0, 4($sp) #restores the original a0 address
    addi $a0, $a0, 29 #a0 ->"FRIEND"

    friend_loop:
      lbu $t0, 0($a0) #t0= friend[i] 
      lbu $t1, 0($a3) #t1= prop_name[i]
      beq $t0, $0, post_friend_loop
      bne $t0, $t1, invalid_prop_name 
      addi $a0, $a0, 1 #friend[i++]
      addi $a3, $a3, 1 #prop_name[i++]
      j friend_loop
    
    post_friend_loop:
      bne $t0, $t1, relation_does_not_exist
      j add_friendship

    add_friendship:
      lw $a0, 4($sp) #restores the original a0 address

      lbu $t0, 20($a0) #t0=current num of edges
      lbu $t1, 12($a0) #t1=size of edges 
      mult $t0, $t1		
      mflo $t0	#$t0= edge bound 

      lbu $t2, 0($a0)#t0=total # of nodes
      lbu $t3, 8($a0)#t1=size of node
      mult $t2, $t3 
      mflo $t2, #t0=total bytes for node

      addi $t2, $t2, 36 #t0=total bytes for nodes+etc
      add $a0, $a0, $t2 #$a0 -> set of edges 
      
      traverse_edge:
        sw $a0, 16($sp) #stores current edge address 
        check_person1:
        lw $t2, 0($a0) #t2=address of p1
        beq $a1, $t2, check_A2
        beq $a2, $t2, check_A1
        j post_check_person1

        check_A2:
          lw $t2, 4($a0) #t2= address of p2
          beq $a2, $t2, found_match #found match
          j post_check_person1

        check_A1:
          lw $t2, 4($a0) #t2=addres of p2
          beq $a1, $t2, found_match #found match 
          j post_check_person1
      
      post_check_person1:
        lw $a0, 16($sp) #restores current edge address
        add $a0, $a0, $t1 #edge[i++]
        sub $t0, $t0, $t1 #counter--
        j traverse_edge
      
    found_match:
      li $t0, 1
      li $v0, 1
      sw $t0, 8($a0)
      j add_relation_property_done


  relation_does_not_exist:
    li $v0, 0
    j add_relation_property_done

  invalid_prop_name:
    li $v0, -1 
    j add_relation_property_done

  add_relation_property_done:
    lw $ra, 0($sp)
    addi $sp, $sp, 20
    jr $ra




.globl is_friend_of_friend
is_friend_of_friend:
  addi $sp, $sp, -32 #allocate space for stack pointer
  sw $ra, 0($sp) #stores the original $ra address 
  sw $a0, 4($sp) #stores original network address 
  sw $a1, 8($sp) #stores name1
  sw $a2, 12($sp) #stores name 2

  jal get_person #gets the node of name 1
  move $t8, $v1 #t8= name 1 node 
  lw $a0, 4($sp) #restores the original network address
  sw $t8, 20($sp) #stores node of name 1

  move $a1, $a2 #a1=name 2
  jal get_person #gets the address node of name 2
  move $t9, $v1 #t9= name 2
  lw $a0, 4($sp) #restores the original network address
  lw $a1, 8($sp) #restores name1

  check_both_name_exist:
    jal is_person_name_exists #check is name1 exist in network 
    beqz $v0, both_name_exist_error #if name1 doesn't exist, error
    lw $a0, 4($sp) #restores the original network address
    move $a1, $a2 #$a1=name 2
    jal is_person_name_exists #check is name 2 exist in network 
    beqz $v0, both_name_exist_error #if name 2 doesn't exist, error
    lw $a0, 4($sp) #restores the original network address
    lw $a1, 8($sp) #restores name1
    j find_is_friend_of_friend

  find_is_friend_of_friend:
    lw $a1, 20($sp) #a1=name 1 node
    move $a2, $t9 #a2=name2 node
    jal check_if_frienship_exists
    bnez $t7, no_friend_of_friend_found
    lw $a0, 4($sp) #restores the original network address
    lw $a1, 20($sp) #a1=name 1 node
    move $a2, $t9 #a2=name2 node

    lw $t0, 0($a0) #t0=total nodes
    lw $t1, 8($a0) #t1=size of nodes
    mult $t1, $t0
    mflo $t0 #t0=node bound 

    lw $t1 20($a0) #t1=curr number of edges
    lw $t2, 12($a0) #t2=size of edges 
    sw $t2, 24($sp) #stores the size of edges 
    mult $t1, $t2, 
    mflo $t1 #t1=edge bound 
    sw $t1, 28($sp) #stores the value of edge bound in stack pointer

    addi $a0, $a0, 36 #a0->node
    add $a0, $a0, $t0 #a0->edge 

    find_match_loop:
      sw $a0, 16($sp) #stores the current network address 
      beqz $t1, no_friend_of_friend_found #no friend of friend found 
      lw $t3, 0($a0) #t1=network_name1[i]
      beq $t3, $a1, check_if_mutuals_name2 #if network_name1=name1, check if connection is mutual
      lw $t3, 4($a0) #t1=network_name2[i]
      beq $t3, $a1, check_if_mutuals_name1 #if network_name2=name1, check if connection is mutual
      j post_find_match_loop

    check_if_mutuals_name2:
      lw $a1, 4($a0) #a1=edge_name_2
      lw $a0, 4($sp) #restores the original network address
      jal is_relation_exists 
      bnez $v0, friend_of_friend_found #common mutual found 
      lw $a1, 20($sp) #restores name 1 node
      j post_find_match_loop


    check_if_mutuals_name1:
      lw $a1, 0($a0) #a1=edge_name1
      lw $a0, 4($sp) #restores the original network address
      jal is_relation_exists
      bnez $v0, friend_of_friend_found #common mutual found 
      lw $a1, 20($sp) #restores name 1 node
      j post_find_match_loop


    post_find_match_loop:
      lw $a0, 16($sp) #restores the current network address 
      lw $t2, 24($sp) #restores the edges size 
      add $a0, $a0, $t2 #edge[i++]
      lw $t1, 28($sp) #restores edge bound
      sub $t1, $t1, $t2 #counter--
      j find_match_loop


  
  both_name_exist_error:
    li $v0, -1 
    j is_friend_of_friend_done
  
  no_friend_of_friend_found:
    li $v0, 0
    j is_friend_of_friend_done

  friend_of_friend_found:
    li $v0, 1
    j is_friend_of_friend_done
  

  is_friend_of_friend_done:
    lw $ra, 0($sp) #restores the original $ra value
    addi $sp, $sp, 32 #deallocate stack pointer
    jr $ra


  check_if_frienship_exists:
    addi $sp, $sp, -4
    sw $ra, 0($sp) 
    jal is_relation_exists
    lw $t7,8($a0) 
    lw $ra, 0($sp)
    addi $sp, $sp, 4
    jr $ra 