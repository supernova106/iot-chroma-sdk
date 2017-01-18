// KeyboardPage.cpp : implementation file
//

#include "stdafx.h"
#include "RazerChromaSampleApplication.h"
#include "WebsocketPage.h"
#include "afxdialogex.h"
#include "TutorialDlg.h"
#include "HealthAmmoManaDlg.h"
#include "AlertDlg.h"
#include "CooldownTimerDlg.h"
#include "AmbientEffectDlg.h"

// websocketapp
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

// json

// regex
#include <regex>

typedef websocketpp::client<websocketpp::config::asio_client> client;

class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
		: m_id(id)
		, m_hdl(hdl)
		, m_status("Connecting")
		, m_uri(uri)
		, m_server("N/A")
	{}

	void on_open(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Open";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
		
		websocketpp::lib::error_code ec;
		std::string subscribe_packet = "";
		c->send(hdl, subscribe_packet, websocketpp::frame::opcode::text, ec);
		if (ec) {
			std::cout << "Echo failed because: " << ec.message() << std::endl;
		}
	}

	void on_fail(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Failed";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
		m_error_reason = con->get_ec().message();
	}

	void on_close(client * c, websocketpp::connection_hdl hdl) {
		m_status = "Closed";
		client::connection_ptr con = c->get_con_from_hdl(hdl);
		std::stringstream s;
		s << "close code: " << con->get_remote_close_code() << " ("
			<< websocketpp::close::status::get_string(con->get_remote_close_code())
			<< "), close reason: " << con->get_remote_close_reason();
		m_error_reason = s.str();
	}

	void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
		if (msg->get_opcode() == websocketpp::frame::opcode::text) {
			m_messages.push_back("<< " + msg->get_payload());
		}
		else {
			m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
		}

		CChromaSDKImpl myChromaObj;
		std::string actual_msg;
		std::string test = msg->get_payload();

		std::regex msg_value("^.*message\\\":\\\"([^\\\"]*).*");
		std::smatch match;
		if (std::regex_match(test, match, msg_value))
		{
			if (match.size() == 2) {
				std::ssub_match base_sub_match = match[1];
				actual_msg = base_sub_match.str();

				std::transform(actual_msg.begin(), actual_msg.end(), actual_msg.begin(), ::toupper);
				COLORREF myColorRef;
				if (actual_msg == "RED")
				{
					myChromaObj.ShowColor(1, RED);		
				}
				else if (actual_msg == "GREEN")
				{
					myChromaObj.ShowColor(1, GREEN);
				}
				else if (actual_msg == "BLUE")
				{
					myChromaObj.ShowColor(1, BLUE);
				}
				else if (actual_msg == "BLINK")
				{
					for (int k = 0; k < 10; k++)
					{
						myColorRef = RGB(
							(BYTE)(rand() % 255), // red component of color
							(BYTE)(rand() % 255), // green component of color
							(BYTE)(rand() % 255) // blue component of color
						);
						myChromaObj.ShowColor(1, myColorRef);
						Sleep(100);
						myChromaObj.ResetEffects(1);
						Sleep(1000);
					}
				}
				else {
					char c_holder[1024];
					strncpy(c_holder, actual_msg.c_str(), sizeof(c_holder));
					c_holder[sizeof(c_holder) - 1] = 0;

					// display each key
					for (const char &value : c_holder)
					{
						if (value == '\0') {
							break;
						}

						int temp;
						temp = int(value);
						UINT VKeys[1] = {
							temp
						};

						
						for (int k = 0; k < 100; k++)
						{
							myColorRef = RGB(
								(BYTE)(rand() % 255), // red component of color
								(BYTE)(rand() % 255), // green component of color
								(BYTE)(rand() % 255) // blue component of color
							);
							myChromaObj.ShowKeys(1, 2, VKeys, myColorRef, FALSE);
						}
						Sleep(150);
					}
					myChromaObj.ResetEffects(1);
				}
			}
		}

		
		/*
		if (actual_msg == "W") {
			UINT VKeys[1] = {
				87          // W
			};
			
			myChromaObj.ShowKeys(1, 1, VKeys, RGB(0, 255, 0), TRUE);
		}
		else if (actual_msg == "A")
		{
			
			UINT VKeys[1] = {
				{ 0x41 }             // A
			};

			myChromaObj.ShowKeys(1, 1, VKeys, RGB(0, 255, 0), TRUE);
		}
		else {
			myChromaObj.ShowColor(1, RED);
			Sleep(150);
			myChromaObj.ResetEffects(1);
		}
		*/
	}

	websocketpp::connection_hdl get_hdl() const {
		return m_hdl;
	}

	int get_id() const {
		return m_id;
	}

	std::string get_status() const {
		return m_status;
	}

	void record_sent_message(std::string message) {
		m_messages.push_back(">> " + message);
	}

	friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
	int m_id;
	websocketpp::connection_hdl m_hdl;
	std::string m_status;
	std::string m_uri;
	std::string m_server;
	std::string m_error_reason;
	std::vector<std::string> m_messages;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
	out << "> URI: " << data.m_uri << "\n"
		<< "> Status: " << data.m_status << "\n"
		<< "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
		<< "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
	out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

	std::vector<std::string>::const_iterator it;
	for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
		out << *it << "\n";
	}

	return out;
}

class websocket_endpoint {
public:
	websocket_endpoint() : m_next_id(0) {
		m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
		m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

		m_endpoint.init_asio();
		m_endpoint.start_perpetual();

		m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
	}

	~websocket_endpoint() {
		m_endpoint.stop_perpetual();

		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			}

			std::cout << "> Closing connection " << it->second->get_id() << std::endl;

			websocketpp::lib::error_code ec;
			m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
			if (ec) {
				std::cout << "> Error closing connection " << it->second->get_id() << ": "
					<< ec.message() << std::endl;
			}
		}

		m_thread->join();
	}

	int connect(std::string const & uri) {
		websocketpp::lib::error_code ec;

		client::connection_ptr con = m_endpoint.get_connection(uri, ec);

		if (ec) {
			std::cout << "> Connect initialization error: " << ec.message() << std::endl;
			return -1;
		}

		int new_id = m_next_id++;
		connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
		m_connection_list[new_id] = metadata_ptr;

		con->set_open_handler(websocketpp::lib::bind(
			&connection_metadata::on_open,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_fail_handler(websocketpp::lib::bind(
			&connection_metadata::on_fail,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_close_handler(websocketpp::lib::bind(
			&connection_metadata::on_close,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_message_handler(websocketpp::lib::bind(
			&connection_metadata::on_message,
			metadata_ptr,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2
		));

		m_endpoint.connect(con);

		return new_id;
	}

	void close(int id, websocketpp::close::status::value code, std::string reason) {
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}

		m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
		if (ec) {
			std::cout << "> Error initiating close: " << ec.message() << std::endl;
		}
	}

	void send(int id, std::string message) {
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}

		m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
		if (ec) {
			std::cout << "> Error sending message: " << ec.message() << std::endl;
			return;
		}

		metadata_it->second->record_sent_message(message);
	}

	connection_metadata::ptr get_metadata(int id) const {
		con_list::const_iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return connection_metadata::ptr();
		}
		else {
			return metadata_it->second;
		}
	}
private:
	typedef std::map<int, connection_metadata::ptr> con_list;

	client m_endpoint;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

	con_list m_connection_list;
	int m_next_id;
};

int WebsocketConnect() {
	bool done = false;
	std::string input = "";
	websocket_endpoint endpoint;

	if (input.substr(0, 7) == "connect") {
		int id = endpoint.connect(input.substr(8));
		if (id != -1) {
			std::cout << "> Created connection with id " << id << std::endl;
		}
	}
	else if (input.substr(0, 4) == "send") {
		std::stringstream ss(input);

		std::string cmd;
		int id;
		std::string message;

		ss >> cmd >> id;
		std::getline(ss, message);

		endpoint.send(id, message);
	}
	else if (input.substr(0, 5) == "close") {
		std::stringstream ss(input);

		std::string cmd;
		int id;
		int close_code = websocketpp::close::status::normal;
		std::string reason;

		ss >> cmd >> id >> close_code;
		std::getline(ss, reason);

		endpoint.close(id, close_code, reason);
	}
	else if (input.substr(0, 4) == "show") {
		int id = atoi(input.substr(5).c_str());

		connection_metadata::ptr metadata = endpoint.get_metadata(id);
		if (metadata) {
			std::cout << *metadata << std::endl;
		}
		else {
			std::cout << "> Unknown connection id " << id << std::endl;
		}
	}
	else {
		std::cout << "> Unrecognized Command" << std::endl;
	}

	return 0;
}

// CWebsocketPage dialog

IMPLEMENT_DYNAMIC(CWebsocketPage, CPropertyPage)

CWebsocketPage::CWebsocketPage()
	: CPropertyPage(CWebsocketPage::IDD)
{

}

CWebsocketPage::~CWebsocketPage()
{
}

void CWebsocketPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CWebsocketPage::OnInitDialog()
{
	BOOL Result = CPropertyPage::OnInitDialog();
	return Result;
}

BEGIN_MESSAGE_MAP(CWebsocketPage, CPropertyPage)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_LOADING_ANIMATION, &CWebsocketPage::OnBnClickedButtonLoadingAnimation)
	ON_BN_CLICKED(IDC_BUTTON_TUTORIAL, &CWebsocketPage::OnBnClickedButtonTutorial)
	ON_BN_CLICKED(IDC_BUTTON_ACTIVE_KEYS, &CWebsocketPage::OnBnClickedButtonActiveKeys)
	ON_BN_CLICKED(IDC_BUTTON_HEALTH_AMMO_MANA, &CWebsocketPage::OnBnClickedButtonHealthAmmoMana)
	ON_BN_CLICKED(IDC_BUTTON_ALERTS, &CWebsocketPage::OnBnClickedButtonAlerts)
	ON_BN_CLICKED(IDC_BUTTON_COOLDOWN_TIMER, &CWebsocketPage::OnBnClickedButtonCooldownTimer)
	ON_BN_CLICKED(IDC_BUTTON_AMBIENT_EFFECT, &CWebsocketPage::OnBnClickedButtonAmbientEffect)
	ON_BN_CLICKED(IDC_BUTTON_DAMAGE_TAKEN, &CWebsocketPage::OnBnClickedButtonDamageTaken)
	ON_BN_CLICKED(IDC_WEBSOCKET_CONNECT, &CWebsocketPage::OnBnClickedWebsocketConnect)
	ON_BN_CLICKED(IDCANCEL, &CWebsocketPage::OnBnClickedCancel)
END_MESSAGE_MAP()


// CWebsocketPage message handlers

void CWebsocketPage::OnDestroy()
{
}

void CWebsocketPage::OnBnClickedButtonLoadingAnimation()
{
	m_ChromaSDKImpl.PlayLoadingAnimation(1);
}

void CWebsocketPage::OnBnClickedButtonTutorial()
{
	CTutorialDlg dlg(this);
	dlg.DoModal();
}

void CWebsocketPage::OnBnClickedButtonActiveKeys()
{
	UINT VKeys[13] = {
		{ 0x41 },             // W
		{ 0x57 },             // A
		{ 0x53 },             // S
		{ 0x44 },             // D
		{ VK_LCONTROL },      // Left Control
		{ VK_LSHIFT },        // Left Shift
		{ VK_SPACE },         // Spacebar
		{ VK_ESCAPE },        // Esc
		{ VK_RETURN },        // Enter
		{ VK_UP },            // Up
		{ VK_DOWN },          // Down
		{ VK_LEFT },          // Left
		{ VK_RIGHT },         // Right
	};

	m_ChromaSDKImpl.ShowKeys(1, 13, VKeys, RGB(0, 255, 0), TRUE);
}

void CWebsocketPage::OnBnClickedButtonHealthAmmoMana()
{
	CHealthAmmoManaDlg dlg(this);
	dlg.DoModal();
}

void CWebsocketPage::OnBnClickedButtonAlerts()
{
	CAlertDlg dlg(this);
	dlg.DoModal();
}

void CWebsocketPage::OnBnClickedButtonCooldownTimer()
{
	CCooldownTimerDlg dlg(this);
	dlg.DoModal();
}

void CWebsocketPage::OnBnClickedButtonAmbientEffect()
{
	CAmbientEffectDlg dlg(this);
	dlg.DoModal();
}

void CWebsocketPage::OnBnClickedButtonDamageTaken()
{
	m_ChromaSDKImpl.ShowColor(1, RED);
	Sleep(50);
	m_ChromaSDKImpl.ResetEffects(1);
}

void CWebsocketPage::OnBnClickedWebsocketConnect()
{
	UINT VKeys[13] = {
		{ 0x41 },             // W
		{ 0x57 },             // A
		{ 0x53 },             // S
		{ 0x44 },             // D
		{ VK_LCONTROL },      // Left Control
		{ VK_LSHIFT },        // Left Shift
		{ VK_SPACE },         // Spacebar
		{ VK_ESCAPE },        // Esc
		{ VK_RETURN },        // Enter
		{ VK_UP },            // Up
		{ VK_DOWN },          // Down
		{ VK_LEFT },          // Left
		{ VK_RIGHT },         // Right
	};

	m_ChromaSDKImpl.ShowKeys(1, 13, VKeys, RGB(0, 255, 0), TRUE);
	WebsocketConnect();
}



void CWebsocketPage::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
}
