# -*- coding: utf8 -*-

import sys
import os
import signal
from subprocess import Popen, PIPE, TimeoutExpired
from time import monotonic as timer


TIMEOUT = 60*60*12 # in seconds
TIMEOUT_GIST = "#gist=to:t/o;to:t/o;to:t/o"
EXAMPLES_DIR = "examples/"

HP = 1
EBR = 2

SMR_NAME = {
	HP: "Hazard Pointers (HP)",
	EBR: "Epoch-Based Reclamation (EBR)",
}
SMR_FILE = {
	HP: "hp.smr",
	EBR: "ebr.smr",
}
SMR_PROGRAM_FOLDER = {
	HP: "HP",
	EBR: "EBR",
}
ADDITIONAL_ARGS = {
	HP: ["-s"],
	EBR: []
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
	cell_time = cells[1].strip()
	return str(cell_time) + "  " + cell_result


def get_type_info(gist):
	return get_info(gist, 0)

def get_annotation_info(gist):
	return get_info(gist, 1)

def get_linearizability_info(gist):
	return get_info(gist, 2)

def run_with_timeout(name, smr, args):
	path_program = EXAMPLES_DIR + SMR_PROGRAM_FOLDER[smr] + "/" + name + ".cola"
	path_smr = EXAMPLES_DIR + SMR_FILE[smr]
	all_args = ['./seal'] + args + ADDITIONAL_ARGS[smr] + [path_program, path_smr]

	# make sure to properly kill subprocesses after timeout
	# see: https://stackoverflow.com/questions/36952245/subprocess-timeout-failure
	start = timer()
	with Popen(all_args, stderr=PIPE, stdout=PIPE, preexec_fn=os.setsid, universal_newlines=True) as process:
		try:
			gist = process.communicate(timeout=TIMEOUT)[0]
		except TimeoutExpired:
			os.killpg(process.pid, signal.SIGINT) # send signal to the process group
			gist = TIMEOUT_GIST
	return gist

def run_test(name, smr):
	gist_types = run_with_timeout(name, smr, ['-gt'])
	gist_annot = run_with_timeout(name, smr, ['-ga'])
	gist_linea = run_with_timeout(name, smr, ['-gl'])
	print("{:<48}   |   {:>15}   |   {:>15}   |   {:>15} ".format(
		SMR_PROGRAM_FOLDER[smr] + "/" + name,
		get_type_info(gist_types),
		get_annotation_info(gist_annot),
		get_linearizability_info(gist_linea)
	), flush=True)

def print_head(smr):
	print()
	print()
	print("{:<48}   |   {:>15}   |   {:>15}   |   {:>15} ".format(smr, "Types", "Annotations", "Linearizability"), flush=True)
	print("---------------------------------------------------+---------------------+---------------------+----------------------", flush=True)

def main():
	print("(Timeout per task/cell is set to: " + str(TIMEOUT) + "s.)", flush=True)

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
