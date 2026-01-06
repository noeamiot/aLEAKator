# Perform a bottom up "synthesis", one for the aubrac core then one for the
# simple_system implementing it.
# Most of the commands here are the one executed with the synth comnand
# the main difference being, we never call: wreduce, alumacc, abc and
# only conditionnaly memory_map.

set allowed_configs {secure unsecure secfast}
set cfg $::env(CONFIG)
if {[info exists env(CONFIG)]} {
  set cfg $::env(CONFIG)
  if {[lsearch -nocase $allowed_configs $cfg] < 0} {
    error "ERROR: Invalid config '$cfg'."
  }
} else {
  error "ERROR: CONFIG environment is mandatory."
}
#set cfg "unsecure"
yosys -import
echo on
read_verilog -defer ./aubrac_${cfg}_gen_verilog/*.v

# Concretize parameters for our top
hierarchy -check -top \$abstract\\simple_system

# First "synthesis", select the aubrac as our top
select -module Aubrac
flatten;
procs;
pmuxtree;
#setattr -set keep 1 c:$memrd;
#select -clear;
opt_expr;
opt_clean;;;

check;
opt -nodffe -nosdff;
fsm;
opt;
peepopt;
opt_clean;
share;
opt;
memory -nomap;
opt_clean;

opt -fast -full;
memory_map;
opt -full;
opt_clean;
dffunmap;

# We want to keep everything that has been produced here
setattr -set keep 1 w:*;
select -clear;
stat;

# Second "synthesis", select our real top as top
hierarchy -check -top simple_system;

flatten;
procs;
pmuxtree;

#select -module simple_system u_top.m_io.m_hpm.m_rdcycle_0.r_shift_*
#splitnets -format @@;
#select -clear;

select -module simple_system ro_io_b_port_0_req_ctrl_addr u_top.m_io.m_hpm.r_hpc_0_w100rdcycle u_top.m_io.m_hpm.r_hpc_0_w100instret u_top.m_pipe.m_back.m_ex.m_muldiv._m_reg_io_b_out_data_uhrem u_top.m_pipe.m_back.m_ex.m_muldiv._m_divss_io_o_rem u_top.m_pipe.m_back.m_ex.m_muldiv._m_ack_io_b_out_data u_top.m_io._m_pma_io_b_mmap_read_data u_top.m_pipe.m_back.m_wb._m_sload_io_b_out_ctrl_info_pc u_top.m_pipe.m_back._m_mem_io_b_out_ctrl_info_pc u_top.m_pipe.m_back._m_ex_io_b_out_ctrl_info_pc u_top.m_pipe._m_back_io_o_br_new_addr u_top.m_pipe._m_back_io_o_br_info_pc u_top.m_pipe.m_front.m_if3.m_buf.r_fifo_1_ctrl_pc u_top.io_b_imem_req_ctrl_addr u_top.m_pipe.m_front.m_pc.r_pc_next u_top.m_pipe.m_front._m_if0_io_b_out_ctrl_pc u_top.m_pipe._m_front_io_b_out_0_ctrl_pc 
splitnets -format @@;
select -clear;

select -module simple_system u_top_ram.m_ctrl_1.m_req.m_reg._m_back_io_b_out_ctrl_addr u_top.m_pipe.m_nlp.r_rsb_wtarget u_top.m_pipe.m_back._m_mem_io_b_out_ctrl_trap_cause u_top.m_pipe.m_back._m_id_io_b_out_ctrl_lsu_uop u_top.m_l0dcross.m_snode_3.r_fifo_1_ctrl_op u_top.m_l0dcross.m_snode_2.r_fifo_1_ctrl_op u_top.m_l0dcross.m_snode_1.r_fifo_1_ctrl_op u_top.m_l0dcross.m_snode_0.r_fifo_1_ctrl_op u_top.m_l0dcross.m_mnode_0.r_fifo_1_ctrl_op u_top.m_io.m_pma.r_pma_3_max u_top.m_io.m_pma.r_pma_2_max u_top.m_io.m_pma.r_pma_1_max u_top_ram.m_ctrl_1.m_req.m_reg.m_back.io_b_out_ctrl_addr u_top_ram.m_ctrl_1.m_req.m_reg.m_back.r_reg_0_ctrl_addr 
splitnets -format @@;
select -clear;

# For S0 only
if { $cfg eq "secure" } {
  select -module simple_system u_top.m_pipe.m_back.m_ex.m_muldiv.r_fsm
  splitnets -format @@;
  select -clear;
}

#select -module simple_system u_top.m_pipe.m_back.m_ex.m_muldiv._m_reg_io_b_out_data_op_0 u_top.m_pipe.m_back.m_ex.m_muldiv._GEN_0 u_top.m_pipe.m_back.m_fsm.r_reg_state u_top.m_pipe.m_back.m_ex*
#splitnets -format @@;
#select -clear;



renames simple_system top;

opt_expr;
opt_clean;;;

check;
opt -nodffe -nosdff;
fsm;
opt;
peepopt;
opt_clean;
share;
opt;
memory -nomap;
opt_clean;


opt -full;
opt_clean -purge;

# Transform dff to basic types
dffunmap;

# Transform all wires to public wires
renames -enumerate;

splitnets -format @@ w:_4837_
splitnets -format @@ w:_4838_
splitnets -format @@ w:_4843_
splitnets -format @@ w:_4846_

# For S1 only
if { $cfg eq "secfast" } {
  splitnets -format @@ w:_5040_
  splitnets -format @@ w:_5041_
  splitnets -format @@ w:_5044_
}

# Write our final cxxrtl model
write_cxxrtl -nohierarchy -noflatten -noproc -O6 -g4 -print-symbolic -header ./cxxrtl_aubrac_${cfg}.cpp;
write_table aubrac_${cfg}.tsv

stat;
