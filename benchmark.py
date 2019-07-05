# -*- coding: latin-1 -*-

import subprocess


def get_cell(gist, i):
	gist = gist.split('=')[-1]
	return gist.split(';')[i]

def get_verdict(verdict):
	result = "-"
	if verdict == '0': result = "✗"
	if verdict == '1': result = "✓"
	if verdict == '?': result = "?"
	return result

def get_info(gist, i):
	cells = get_cell(gist, i)
	cells = cells.split(':')
	cell_result = get_verdict(cells[0])
	cell_time = cells[1]
	return "{:>10}".format(cell_time) + "  " + cell_result

def get_type_info(gist):
	number_guarantees = get_cell(gist, 0)
	return get_info(gist, 1) + "  (" + "{:>3}".format(number_guarantees) + ")"

def get_annotation_info(gist):
	return get_info(gist, 2)

def get_linearizability_info(gist):
	return get_info(gist, 3)

def run_test(filename):
	process_synth = subprocess.Popen(['./seal', '-gt', filename], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	process_hand = subprocess.Popen(['./seal', '-gtcal', filename], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	out_synth, err_synth = process_synth.communicate()
	out_hand, err_hand = process_hand.communicate()
	
	gist_synth = out_synth.splitlines()[-1]
	gist_hand = out_hand.splitlines()[-1]
	# print out_hand

	print "{:<50}".format(filename), "   |   ", get_type_info(gist_synth), "   |   ", get_type_info(gist_hand), "   |   ", get_annotation_info(gist_hand), "   |   ", get_linearizability_info(gist_hand)

def print_head(smr):
	print "{:<50}    |  {:>22}    |  {:>22}    |  {:>15}    |  {:>15} ".format(smr + " Program", "hand-crafted Types", "synthesized Types", "Annotations", "Linearizability")
	print "------------------------------------------------------+----------------------------+----------------------------+---------------------+------------------"

def main():
	# HP
	print_head("HP")
	run_test("../../examples/HP/TreiberStack_transformed.cola")
	run_test("../../examples/HP/TreiberOptimizedStack_transformed.cola")
	run_test("../../examples/HP/MichaelScottQueue_transformed.cola")
	run_test("../../examples/HP/DGLM_transformed.cola")
	run_test("../../examples/HP/VechevDCasSet_transformed.cola")
	run_test("../../examples/HP/VechevCasSet_transformed.cola")
	run_test("../../examples/HP/OHearnSet_transformed.cola")
	run_test("../../examples/HP/MichaelSet_transformed.cola")

	# # EBR
	# print_head("EBR")
	# run_test("../../examples/EBR/TreiberStack.cola")
	# run_test("../../examples/EBR/TreiberOptimizedStack.cola")
	# run_test("../../examples/EBR/MichaelScottQueue.cola")
	# run_test("../../examples/EBR/DGLM.cola")
	# run_test("../../examples/EBR/VechevDCasSet.cola")
	# run_test("../../examples/EBR/VechevCasSet.cola")
	# run_test("../../examples/EBR/OHearnSet.cola")
	# run_test("../../examples/EBR/MichaelSet.cola")


if __name__ == '__main__':
	main()