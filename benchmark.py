# -*- coding: utf8 -*-

#################################################
######## Needs to be run from /build/bin ########
#################################################

import sys
from subprocess import STDOUT, check_output, TimeoutExpired

TIMEOUT = 60*60*1 # in seconds
TIMEOUT_GIST = "#gist=?;to:t/o;to:t/o;to:t/o;to:t/o"
EXAMPLES_DIR = "examples/"

HP = 1
EBR = 2
HP_MONOLITH = 3

SMR_NAME = {
	HP: "Hazard Pointers (HP)",
	EBR: "Epoch-Based Reclamation (EBR)",
}
SMR_FILE = {
	HP: "hp_forward_transfer.smr",
	EBR: "ebr.smr",
}
SMR_PROGRAM_FOLDER = {
	HP: "HP",
	EBR: "EBR",
}
TYPE_FILE = {
	HP: "CustomTypes/hp_guarantees_forward_transfer.smr",
	EBR: "CustomTypes/ebr_guarantees.smr",
}


def get_cell(gist, i):
	gist = gist.split('=')[-1]
	return gist.split(';')[i]

def get_verdict(verdict):
	result = "-"
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
	return "{:>16}".format(cell_time) + "  " + cell_result + " "

def get_type_info(gist):
	number_guarantees = get_cell(gist, 0)
	return get_info(gist, 2)

def run_with_timeout(name, smr, args):
	path_program = EXAMPLES_DIR + SMR_PROGRAM_FOLDER[smr] + "/" + name + ".cola"
	path_smr = EXAMPLES_DIR + SMR_FILE[smr]
	all_args = ['./seal'] + args + [path_program, path_smr]

	try:
		out  = check_output(all_args, stderr=STDOUT, timeout=TIMEOUT, text=True)
		gist = out.splitlines()[-1]
	except TimeoutExpired:
		gist = TIMEOUT_GIST
	return gist

def run_test(name, smr):
	gist_synth = run_with_timeout(name, smr, ['-gt'])
	gist_hand = run_with_timeout(name, smr, ['-gt', '-c', EXAMPLES_DIR + TYPE_FILE[smr]])
	print("{:<48}   |   {}   |   {}".format(
		SMR_PROGRAM_FOLDER[smr] + "/" + name,
		get_type_info(gist_synth),
		get_type_info(gist_hand),
	), flush=True)

def print_head(smr):
	print()
	print()
	print("{:<48}   |  {:>20}    |  {:>20}".format(smr, "synthesized Types", "manual Types"), flush=True)
	print("---------------------------------------------------+--------------------------+--------------------------", flush=True)

def main():
	print("(Timeout per task is set to: " + str(TIMEOUT) + "s.)", flush=True)

	# HP
	print_head(SMR_NAME[HP])
	run_test("TreiberStack_transformed", HP)
	run_test("TreiberOptimizedStack_transformed", HP)
	run_test("MichaelScottQueue_transformed", HP)
	run_test("DGLM_transformed", HP)
	run_test("VechevDCasSet_transformed", HP)
	run_test("VechevCasSet_transformed", HP)
	run_test("OHearnSet_transformed", HP)
	run_test("MichaelSet_transformed", HP)

	# EBR
	print_head(SMR_NAME[EBR])
	run_test("TreiberStack", EBR)
	# run_test("TreiberOptimizedStack", EBR)
	run_test("MichaelScottQueue", EBR)
	run_test("DGLM", EBR)
	run_test("VechevDCasSet", EBR)
	run_test("VechevCasSet", EBR)
	run_test("OHearnSet", EBR)
	run_test("MichaelSet", EBR)


if __name__ == '__main__':
	if len(sys.argv) > 1:
		if len(sys.argv) != 2:
			raise Exception("Wrong number of arguments!")
		TIMEOUT = int(sys.argv[1])

	try:
		main()
	except KeyboardInterrupt:
		print("", flush=True)
		print("", flush=True)
		print("[interrupted]", flush=True)
