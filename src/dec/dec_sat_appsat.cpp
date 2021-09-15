

#include "dec/dec_sat.h"
#include "main/main_util.h"

namespace dec_ns {

using namespace utl;

void dec_sat_appsat::solve() {

	print_appsat_params();

	_init_handlers();
	_prepare_sat_attack();

	start_time = utl::_start_wall_timer();

	// Decamouflage Loop
	while ( true ) {

		iteration++;
		print_stats(iteration);

		iopair_t dp;
		int ret = solve_for_dip(dp);

		if (ret == 0) {
			break;
		}

		// Record observations in SAT
		dp.y = query_oracle(dp.x);
		create_ioconstraint(dp, iovecckt);

		// check iteration cap
		if (iteration_limit != -1
				&& iteration >= iteration_limit) {
			std::cout << "iteration limit reached\n";
			break;
		}

		// intermediate error analysis
		if (iteration % num_dip_period == 0)
			if (analyze_inter_key())
				break;

		/* termination condition checking
		if (check_term_condition())
			break;
		*/
	}

	// Extract key from SAT
	solve_key();

	return;
}

// option parser
void dec_sat_appsat::get_opt_parser(dec_sat_appsat& dc, boost::program_options::options_description& op) {

	dec_sat::get_opt_parser(dc, op);
	dec_sat_appsat::append_opt_parser(dc, op);

}

void dec_sat_appsat::append_opt_parser(dec_sat_appsat& dc, boost::program_options::options_description& op) {

	namespace po = boost::program_options;

	op.add_options()
		("appsat_reinforce", po::bool_switch(&dc.appsat_reinforce), "reinforce queries")
		("appsat_numDipQueries", po::value<int>(&dc.num_dip_period), "AppSAT number of DIP queries before error estimation")
		("appsat_Thr", po::value<int>(&dc.appssat_error_threshold), "AppSAT error threshold for termination");

}

void dec_sat_appsat::print_usage_message() {
	std::cout << "usage: neos -d app <input_file> <output_file> <iteration_limit>\n";
	std::cout << "usage: neos -d app <input_file> <output_file>"
			" <iteration_limit> <num_rand_q> "
			"<num_dip_q> <error_threshold> <settlement_steps>\n";
}

int dec_sat_appsat::parse_args(int argc, char** argv) {

	namespace po = boost::program_options;
	po::options_description op;
	dec_sat_appsat::get_opt_parser(*this, op);

	if (mnf::parse_args(argc, argv, op) || print_help) {
		print_usage_message();
		return 1;
	}

	read_sim_enc_key_args(pos_args, !oracle_binary.empty());

	if (pos_args.size() == 1) {
		iteration_limit = stoi(pos_args[0]);
	}
	else if (pos_args.size() == 5) {
		iteration_limit = stoi(pos_args[0]);
		num_rand_queries = stoi(pos_args[1]);
		num_dip_period = stoi(pos_args[2]);
		appssat_error_threshold = stoi(pos_args[3]);
		appsat_Settle = stoi(pos_args[4]);
		return 0;
	}
	else {
		print_usage_message();
		return 1;
	}

    return 0;
}


void dec_sat_appsat::print_appsat_params() {
	std::cout << "\nappsat params: ";
	std::cout << "\nnum dip queries before error calc: " << num_dip_period;
	std::cout << "\nnum random queries during error calc:" << num_rand_queries;
	std::cout << "\nsettlement threshold: " << appssat_error_threshold;
	std::cout << "\nminimum settlement steps: " << appsat_Settle << "\n";
}

bool dec_sat_appsat::analyze_inter_key() {

	get_inter_key();

	// for small circuits sweep entire input space
	uint64_t err = get_key_error(interkey, 0, num_rand_queries);

	queries += num_rand_queries;

	double elap_time = utl::_stop_wall_timer(start_time);

	std::cout << std::setw(5) << "cur_t: " << std::setw(9) << elap_time << "; ";
	std::cout << std::setw(5) << "error: " << std::setw(5) << err << "; ";

	if (err == 0) {
		low_hd_count++;
		if (low_hd_count >= appsat_Settle) {
			queries += iteration;
			std::cout << "\nconverged!\n";
			std::cout << "querie=" << queries
					  << "  iteration=" << iteration << "  key=";
			printvec(interkey);
			return true;
		}
	}
	else {
		low_hd_count = 0;
	}

	return false;
}


}
