#include "accept_manager.h"

accept_manager::accept_manager(int port) : io_(), acceptor_(io_, tcp::endpoint(tcp::v4(), port)) {
	accept();

	run_flag_ = true;
	run_th_ = std::thread(&accept_manager::run, this);
	check_client_th_ = std::thread(&accept_manager::check_client, this);
}

accept_manager::~accept_manager() {
	io_.stop();
	run_flag_ = false;

	if (run_th_.joinable()) {
		run_th_.join();
	}
	
	if (check_client_th_.joinable()) {
		check_client_th_.join();
	}

	for (auto it : clients_) {
		delete it;
	}
	clients_.clear();
}

void accept_manager::run() {
	io_.run();
}

void accept_manager::accept() {
	api_client *cli = new api_client(io_);
	acceptor_.async_accept(cli->socket(), 
		[this, cli](const boost::system::error_code &ec)
		{
			if (ec) {
				g_data::log(ERROR_LOG_LEVEL, "[accept_manager] Accept fail.[%s]", ec.message().c_str());
				delete cli;
			} else {
				g_data::log(INFO_LOG_LEVEL, "[accept_manager] Accept from client.");
				cli->run();

				std::lock_guard<std::mutex> guard(client_mutex_);
				clients_.push_back(cli);
			}

			accept();
		}
	);
}

void accept_manager::check_client() {
	while (run_flag_) {
		std::lock_guard<std::mutex> guard(client_mutex_);

		for (auto it = clients_.begin(); it != clients_.end();) {
			if ((*it)->is_run() == false) {
				delete *it;
				it = clients_.erase(it);
			} else {
				++it;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}