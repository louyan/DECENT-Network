/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/make_shared.hpp>



#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <iostream>
#include <fstream>
#include <atomic>

#include <decent/encrypt/encryptionutils.hpp>


#include <libtorrent/alert_types.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>


#include "torrent_transfer.hpp"


using namespace graphene::package;
using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace libtorrent;


namespace {

	int load_file(std::string const& filename, std::vector<char>& v, libtorrent::error_code& ec, int limit = 8000000) {
		ec.clear();
		FILE* f = std::fopen(filename.c_str(), "rb");
		if (f == nullptr) {
			ec.assign(errno, boost::system::system_category());
			return -1;
		}

		int r = fseek(f, 0, SEEK_END);
		if (r != 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		long s = ftell(f);
		if (s < 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		if (s > limit) {
			std::fclose(f);
			return -2;
		}

		r = fseek(f, 0, SEEK_SET);
		if (r != 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		v.resize(s);
		if (s == 0) {
			std::fclose(f);
			return 0;
		}

		r = int(std::fread(&v[0], 1, v.size(), f));
		if (r < 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		std::fclose(f);

		if (r != s) return -3;
		return 0;
	}

	int save_file(std::string const& filename, std::vector<char>& v) {
		FILE* f = std::fopen(filename.c_str(), "wb");
		if (f == nullptr)
			return -1;

		int w = int(std::fwrite(&v[0], 1, v.size(), f));
		std::fclose(f);

		if (w < 0) return -1;
		if (w != int(v.size())) return -3;
		return 0;
	}


}


torrent_transfer::torrent_transfer() {
	_my_thread = new fc::thread("torrent_thread");
	libtorrent::settings_pack p = _session.get_settings();
	p.set_bool(libtorrent::settings_pack::enable_lsd, true);
	p.set_bool(libtorrent::settings_pack::enable_upnp, true);
	p.set_bool(libtorrent::settings_pack::enable_natpmp, true);
	p.set_bool(libtorrent::settings_pack::enable_dht, true);
	p.set_bool(libtorrent::settings_pack::allow_multiple_connections_per_ip, true);

	p.set_int(libtorrent::settings_pack::dht_announce_interval, 15);
	p.set_int(libtorrent::settings_pack::active_limit, 100);
	p.set_int(libtorrent::settings_pack::active_seeds, 90);
	p.set_int(libtorrent::settings_pack::active_downloads, 10);

	p.set_str(libtorrent::settings_pack::user_agent, "DECENT");
	
	_session.apply_settings(p);

	libtorrent::error_code ec;

	std::vector<char> in;
	if (load_file(".ses_state", in, ec) == 0) {
		bdecode_node e;
		if (bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
			_session.load_state(e, session::save_dht_state);
	}

	_session.set_alert_notify([this]() {

		_my_thread->async([this] () {
        	this->handle_torrent_alerts();           
        });

    });

}

torrent_transfer::~torrent_transfer() {
	entry session_state;
	_session.save_state(session_state, session::save_dht_state);

	std::vector<char> out;
	bencode(std::back_inserter(out), session_state);
	save_file(".ses_state", out);

	delete _my_thread;
}

void torrent_transfer::print_status() {
	libtorrent::torrent_status st = _torrent_handle.status();
	cout << "Error Message/String/File: " << st.errc.message() << " / " << st.error << " / " << st.error_file << endl;
	cout << "Save path/Name/Current Tracker: " << st.save_path << " / " << st.name << " / " << st.current_tracker << endl;
	cout << "Total download/upload/payload download/payload upload: " << st.total_download << " / " << st.total_upload << " / " << st.total_payload_download << " / " << st.total_payload_upload << endl;
	cout << "Download rate/Upload rate/Seeds/Peers: " << st.download_rate << " / " << st.upload_rate << " / " << st.num_seeds << " / " << st.num_peers << endl;
	cout << "Distributed Full Copies/Distributed Fraction/Distributed Copies: " << st.distributed_full_copies << " / " << st.distributed_fraction << " / " << st.distributed_copies << endl;
	cout << "Active Time/Finished Time/Seeding Time: " << st.active_time  << " / " << st.finished_time << " / " << st.seeding_time << endl;
	cout << "Num Uploads/Num Connections/Uploads Limit/Connections Limit: " << st.num_uploads << " / " << st.num_connections << " / " << st.uploads_limit << " / " << st.connections_limit << endl;
	cout << "Next announce/Progress/State: " << st.next_announce.count() << " / " << st.progress << "/";

	switch (st.state) {
		case torrent_status::checking_files:
			cout << "checking_files" << endl;
			break;
		case torrent_status::downloading_metadata:
			cout << "downloading_metadata" << endl;
			break;
		case torrent_status::downloading:
			cout << "downloading" << endl;
			break;
		case torrent_status::finished:
			cout << "finished" << endl;
			break;
		case torrent_status::seeding:
			cout << "seeding" << endl;
			break;
		case torrent_status::allocating:
			cout << "allocating" << endl;
			break;
		case torrent_status::checking_resume_data:
			cout << "checking_resume_data" << endl;
			break;
		case torrent_status::queued_for_checking:
			cout << "queued_for_checking" << endl;
			break;
	}

	cout << "Upload Mode/Share Mode/Super Seeding/Auto Managed: " << st.upload_mode << " / " << st.share_mode << " / " << st.super_seeding << " / " << st.auto_managed << endl;
	cout << "Paused/Seeding?/Finished?/Loaded?: " << st.paused << " / " << st.is_seeding << " / " << st.is_finished << " / " << st.is_loaded << endl;
	cout << "Has Metadata/Has Incoming/Stop When Ready: " << st.has_metadata << " / " << st.has_incoming << " / " << st.stop_when_ready << endl;
	cout << "Announcing To Trackers/LSD/DHT: " << st.announcing_to_trackers << " / " << st.announcing_to_lsd << " / " << st.announcing_to_dht << endl;
	cout << "Seed Mode/Seed Rank: " << st.seed_mode << " / " << st.seed_rank << endl;
	cout << "Block Size/Num Pieces: " << st.block_size << " / " << st.num_pieces<< endl;

}


void torrent_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
	_id = id;
	_listener = listener;

	file_storage fs;
	libtorrent::error_code ec;

	// recursively adds files in directories
	add_files(fs, package.get_path().string());

	create_torrent t(fs, 5 * 64 * 16 * 1024); // 5MB pieces
	t.set_creator("DECENT");
    t.set_priv(false);

	// reads the files and calculates the hashes
	set_piece_hashes(t, package.get_path().parent_path().string(), ec);

	if (ec) {
		listener->on_error(_id, ec.message());
		return;
	}

	path temp_file = temp_directory_path() / (package.get_hash().str() + ".torrent");
	path temp_dir = temp_directory_path();

	cout << "Torrent file created: " << temp_file.string() << endl;

	std::ofstream out(temp_file.string(), std::ios_base::binary);
	bencode(std::ostream_iterator<char>(out), t.generate());
	out.close();

	libtorrent::add_torrent_params atp;

	std::shared_ptr<libtorrent::torrent_info> ptt = std::make_shared<libtorrent::torrent_info>(temp_file.string(), 0);

	atp.ti = ptt;
	atp.flags = libtorrent::add_torrent_params::flag_seed_mode |
				libtorrent::add_torrent_params::flag_upload_mode |
				//libtorrent::add_torrent_params::flag_share_mode	|
				libtorrent::add_torrent_params::flag_super_seeding;

	atp.save_path = package.get_path().parent_path().string();


	atp.dht_nodes.push_back(std::make_pair("router.utorrent.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("router.bittorrent.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("dht.transmissionbt.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("router.bitcomet.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("dht.aelitis.com", 6881));

	int num_pieces = atp.ti->num_pieces();


	_torrent_handle = _session.add_torrent(atp);
    _torrent_handle.set_upload_mode(true);
    _torrent_handle.super_seeding(true);
    _torrent_handle.force_dht_announce();


	_url = make_magnet_uri(_torrent_handle);
	cout << "Magnet URL: " << _url << endl;

}

void torrent_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_url = url;

	libtorrent::add_torrent_params atp;
	atp.url = url.c_str();

	path temp_dir = temp_directory_path();

	atp.save_path = temp_dir.string();

	atp.dht_nodes.push_back(std::make_pair("router.utorrent.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("router.bittorrent.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("dht.transmissionbt.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("router.bitcomet.com", 6881));
	atp.dht_nodes.push_back(std::make_pair("dht.aelitis.com", 6881));

	cout << "Save path is: " << atp.save_path << endl;

	_torrent_handle = _session.add_torrent(atp);
}



void torrent_transfer::update_torrent_status() {
	libtorrent::torrent_status st = _torrent_handle.status();

	bool is_finished = st.state == libtorrent::torrent_status::finished || st.state == libtorrent::torrent_status::seeding;
	bool is_error = st.errc != 0;

	if (!is_finished && !is_error) {
		_my_thread->schedule([this] () {
        	this->update_torrent_status();
        }, fc::time_point::now() + fc::seconds(5));
	}
	if (is_error) {
		_listener->on_error(_id, st.errc.message());
	} else {
		_listener->on_download_progress(_id, transfer_progress(st.total_wanted, st.total_wanted_done, st.download_rate));
	}
}


void torrent_transfer::handle_torrent_alerts() {
	
	std::vector<libtorrent::alert*> alerts;
	_session.pop_alerts(&alerts);

	for (int i = 0; i < alerts.size(); ++i) {
		libtorrent::alert* alert = alerts[i];

		switch (alert->type()) {
			case libtorrent::torrent_added_alert::alert_type:
				_listener->on_download_started(_id);

				_my_thread->schedule([this] () {
		        	this->update_torrent_status();           
		        }, fc::time_point::now() + fc::seconds(5));
				break;

			case libtorrent::torrent_finished_alert::alert_type:
            	_listener->on_download_finished(_id, package_manager::instance().get_package_object(fc::ripemd160("22ad84efeca3776a4e37b738eab728abcedc92d8")));
				break;

			//case libtorrent::tracker_error_alert::alert_type:
			//case libtorrent::peer_error_alert::alert_type:
			case libtorrent::file_error_alert::alert_type:
			//case libtorrent::udp_error_alert::alert_type:
			//case libtorrent::portmap_error_alert::alert_type:
			case libtorrent::torrent_error_alert::alert_type:
			//case libtorrent::dht_error_alert::alert_type:
			//case libtorrent::lsd_error_alert::alert_type:
            	_listener->on_error(_id, alert->message()); 
            	break;

			
		}
		
		//cout << "Torrent alert: " << alert->message() << endl;
	}
}



std::string torrent_transfer::get_transfer_url(transfer_id id) {
	return _url;
}

torrent_transfer::transfer_progress torrent_transfer::get_transfer_progress(transfer_id id) {
	return transfer_progress(1000, 800, 120);
}



