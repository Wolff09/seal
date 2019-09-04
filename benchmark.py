# -*- coding: utf8 -*-

#################################################
######## Needs to be run from /build/bin ########
#################################################

from subprocess import STDOUT, check_output, TimeoutExpired

TIMEOUT = 60*60*12 # in seconds
TIMEOUT_GIST = "#gist=?;to:t/o;to:t/o;to:t/o;to:t/o"
HP = "HP"
EBR = "EBR"
HP_MONOLITH = "HP_MONOLITH"
EXAMPLES_DIR = "../../examples/"
SMR_FILE = { HP: "hp_forward_transfer.smr", EBR: "ebr.smr", HP_MONOLITH: "hp_forward_transfer_monolith.smr" }
TYPE_FILE = { HP: "CustomTypes/hp_guarantees_forward_transfer.smr", EBR: "CustomTypes/ebr_guarantees.smr" }


def get_cell(gist, i):
	gist = gist.split('=')[-1]
	return gist.split(';')[i]

def get_verdict(verdict):
	result = "-"
	# if verdict == "0": result = "❌"
	# if verdict == "1": result = "✅"
	# if verdict == "to": result = "⌛"
	if verdict == "0": result = "✗"
	if verdict == "1": result = "✓"
	if verdict == "to": result = "✗"
	if verdict == "?": result = "?"
	return result

def get_info(gist, i):
	cells = get_cell(gist, i)
	cells = cells.split(':')
	cell_result = get_verdict(cells[0])
	cell_time = cells[1]
	return "{:>12}".format(cell_time) + "  " + cell_result

def get_synthesis_info(gist):
	cells = get_cell(gist, 1)
	cells = cells.split(':')
	cell_time = cells[1]
	return "{:>11}".format(cell_time)

def get_type_info(gist):
	number_guarantees = get_cell(gist, 0)
	return get_info(gist, 2) + "  " + "{:>5}".format("(" + number_guarantees + ")")

def get_annotation_info(gist):
	return get_info(gist, 3)

def get_linearizability_info(gist):
	return get_info(gist, 4)

def run_with_timeout(name, smr, args):
	path_program = EXAMPLES_DIR + smr + "/" + name + ".cola"
	path_smr = EXAMPLES_DIR + SMR_FILE[smr]
	all_args = ['./seal'] + args + [path_program, path_smr]

	# TODO: handle timeout
	try:
		out  = check_output(all_args, stderr=STDOUT, timeout=TIMEOUT, text=True)
		gist = out.splitlines()[-1]
	except TimeoutExpired:
		gist = TIMEOUT_GIST
	return gist

def run_test(name, smr):
	gist_synth = run_with_timeout(name, smr, ['-gts'])
	gist_hand = run_with_timeout(name, smr, ['-gts', '-c', EXAMPLES_DIR + TYPE_FILE[smr]])
	gist_annotations = run_with_timeout(name, smr, ['-ga'])
	gist_linear = run_with_timeout(name, smr, ['-gl'])
	print("{:<48}   |   {}   |   {}   |   {}   |   {}   |    {}".format(
		smr + "/" + name,
		get_synthesis_info(gist_synth),
		get_type_info(gist_synth),
		get_type_info(gist_hand),
		get_annotation_info(gist_annotations),
		get_linearizability_info(gist_linear)
	))

def print_head(smr):
	print()
	print("{:<48}   |   {:>11}   |  {:>22}    |  {:>22}    |  {:>15}    |   {:>15} ".format(smr + " Program", "Synthesis", "synthesized Types", "hand-crafted Types", "Annotations", "Linearizability"))
	print("---------------------------------------------------+-----------------+----------------------------+----------------------------+---------------------+----------------------")

def main():
	# HP
	print_head("HP")
	run_test("TreiberStack_transformed", HP)
	# run_test("TreiberOptimizedStack_transformed", HP)
	# run_test("MichaelScottQueue_transformed", HP)
	# run_test("DGLM_transformed", HP)
	# run_test("VechevDCasSet_transformed", HP)
	# run_test("VechevCasSet_transformed", HP)
	# run_test("OHearnSet_transformed", HP)
	# run_test("MichaelSet_transformed", HP)

	# EBR
	print_head("EBR")
	run_test("/TreiberStack", EBR)
	# run_test("/TreiberOptimizedStack", EBR)
	# run_test("/MichaelScottQueue", EBR)
	# run_test("/DGLM", EBR)
	# run_test("/VechevDCasSet", EBR)
	# run_test("/VechevCasSet", EBR)
	# run_test("/OHearnSet", EBR)
	# run_test("/MichaelSet", EBR)

	# HP 
	print_head("HP (monolith)")
	SMR_FILE[HP] = SMR_FILE[HP_MONOLITH]
	run_test("TreiberStack_transformed", HP)
	# run_test("TreiberOptimizedStack_transformed", HP)
	# run_test("MichaelScottQueue_transformed", HP)
	# run_test("DGLM_transformed", HP)
	# run_test("VechevDCasSet_transformed", HP)
	# run_test("VechevCasSet_transformed", HP)
	# run_test("OHearnSet_transformed", HP)
	# run_test("MichaelSet_transformed", HP)


if __name__ == '__main__':
	main()