#include <future>
#include <nghttp2/asio_http2_server.h>
#include <nghttp2/asio_http2_client.h>
#include <Simple-WebSocket-Server/server_wss.hpp>

#include "agent.h"
#include "tcpserver.h"


#include <nghttp2/asio_http2_client.h>


#include <sys/time.h>
#include <time.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
extern "C" {
#include "mw_asr.h"
}
#include "ringbuffer.h"

boost::asio::io_service io_service;
RingBuffer_t g_WakeUpRingBuffer;

void f_web_socket(t_agent& a_agent)
{
	// if (a_agent.v_log) a_agent.v_log(e_severity__TRACE) << "websocket starting..." << std::endl;
	auto& session = *a_agent.f_session();
	std::map<std::string, std::function<void(const picojson::value&)>> handlers{
		{"hello", [&](auto)
		{
			a_agent.v_state_changed();
			a_agent.v_options_changed();
		}},
		{"connect", [&](auto)
		{
			session.f_connect();
		}},
		{"disconnect", [&](auto)
		{
			session.f_disconnect();
		}},
		{"capture.threshold", [&](auto a_x)
		{
			session.f_capture_threshold(a_x.template get<double>());
		}},
		{"capture.auto", [&](auto a_x)
		{
			session.f_capture_auto(a_x.template get<bool>());
		}},
		{"capture.force", [&](auto a_x)
		{
			session.f_capture_force(a_x.template get<bool>());
		}},
		{"alerts.duration", [&](auto a_x)
		{
			session.f_alerts_duration(std::min(static_cast<size_t>(a_x.template get<double>()), size_t(3600)));
		}},
		{"alerts.stop", [&](auto a_x)
		{
			session.f_alerts_stop(a_x.template get<std::string>());
		}},
		{"content.background", [&](auto a_x)
		{
			session.f_content_can_play_in_background(a_x.template get<bool>());
		}},
		{"speaker.volume", [&](auto a_x)
		{
			session.f_speaker_volume(a_x.template get<double>());
		}},
		{"speaker.muted", [&](auto a_x)
		{
			session.f_speaker_muted(a_x.template get<bool>());
		}},
		{"playback.play", [&](auto)
		{
			session.f_playback_play();
		}},
		{"playback.pause", [&](auto)
		{
			session.f_playback_pause();
		}},
		{"playback.next", [&](auto)
		{
			session.f_playback_next();
		}},
		{"playback.previous", [&](auto)
		{
			session.f_playback_previous();
		}}
	};
	
	auto send = [&](const std::string& a_name, picojson::value::object&& a_values)
	{
		auto message = picojson::value(picojson::value::object{
			{a_name, picojson::value(std::move(a_values))}
		}).serialize();
		
		// std::cout << "send " << message << std::endl;
	};
	a_agent.v_capture = [&]
	{
		send("capture", {
			{"integral", picojson::value(static_cast<double>(session.f_capture_integral()))},
			{"busy", picojson::value(session.f_capture_busy())}
		});
	};
	a_agent.v_state_changed = [&]
	{
		picojson::value::array alerts;
		for (auto& x : session.f_alerts()) alerts.push_back(picojson::value(picojson::value::object{
			{"token", picojson::value(x.first)},
			{"type", picojson::value(x.second.f_type())},
			{"at", picojson::value(x.second.f_at())},
			{"active", picojson::value(x.second.f_active())}
		}));
		send("state_changed", {
			{"online", picojson::value(session.f_online())},
			{"dialog", picojson::value(picojson::value::object{
				{"active", picojson::value(session.f_dialog_active())},
				{"playing", picojson::value(session.f_dialog_playing())},
				{"expecting_speech", picojson::value(session.f_expecting_speech())}
			})},
			{"alerts", picojson::value(std::move(alerts))},
			{"content", picojson::value(picojson::value::object{
				{"playing", picojson::value(session.f_content_playing())}
			})}
		});
	};
	a_agent.v_options_changed = [&]
	{
		send("options_changed", {
			{"alerts", picojson::value(picojson::value::object{
				{"duration", picojson::value(static_cast<double>(session.f_alerts_duration()))}
			})},
			{"content", picojson::value(picojson::value::object{
				{"can_play_in_background", picojson::value(session.f_content_can_play_in_background())}
			})},
			{"speaker", picojson::value(picojson::value::object{
				{"volume", picojson::value(static_cast<double>(session.f_speaker_volume()))},
				{"muted", picojson::value(session.f_speaker_muted())}
			})},
			{"capture", picojson::value(picojson::value::object{
				{"threshold", picojson::value(static_cast<double>(session.f_capture_threshold()))},
				{"auto", picojson::value(session.f_capture_auto())},
				{"force", picojson::value(session.f_capture_force())}
			})}
		});
	};

	// if (a_agent.v_log) a_agent.v_log(e_severity__TRACE) << "websocket ready." << std::endl;
}

struct t_null_buffer : std::streambuf
{
	virtual int overflow(int a_c)
	{
		return a_c;
	}
};


int main(int argc, char **argv)
{
	std::cout<<__FUNCTION__<<" "<<__LINE__<<std::endl;

	int a_severity = 0;
	t_null_buffer null_buffer;
	std::ostream null_stream{&null_buffer};
	auto log = [&](auto a_severity) -> std::ostream&
	{
		return std::ref(/*a_severity < severity ? null_stream : */std::cerr);
	};

	picojson::value configuration;
	{
		std::ifstream s(FILE_SESSION_JSON);
		s >> configuration;
	}
	auto profile = configuration / "profile";


	t_agent agent(log, profile, configuration / "sounds");
	
	agent.f_start(io_service, [&]()
	{
		if (agent.f_session()) {
			f_web_socket(agent);
			std::cout<<__FUNCTION__<<" "<<__LINE__<<std::endl;
		}
	});

	boost::asio::signal_set signals(io_service, SIGINT);
	signals.async_wait([&](auto, auto a_signal)
	{
		log(e_severity__INFORMATION) << std::endl << "caught signal: " << a_signal << std::endl;
		agent.f_stop([&]
		{
			std::cout<<__FUNCTION__<<" "<<__LINE__<<std::endl;
		});
		
	});

	std::string cid = profile / "client_id"_jss;
	tcpServerSetClientId(cid);

	std::thread wsthread;
	wsthread = std::thread(tcpServerRun, &agent);

	io_service.run();

	if (wsthread.joinable()) wsthread.join();


	log(e_severity__INFORMATION) << "server stopped." << std::endl;
	return 0;
}



