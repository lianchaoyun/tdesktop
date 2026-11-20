/*
This file is part of a custom tdesktop build.

A small login-time dialog for the self-hosted server address. It writes the
DC2 endpoint into tdata/custom_endpoints.txt and points custom_servers.ini at
it, so the next launch (or a directly launched exe) picks the address up.
*/
#pragma once

#include "core/custom_servers.h"

#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "styles/style_layers.h"
#include "window/window_controller.h"

namespace CustomServers {

inline void ShowAddressBox(
		not_null<Window::Controller*> controller,
		int accountIndex = -1) {
	const auto endpoint = ReadCustomEndpoint();
	const auto fallbackPort = endpoint.port > 0
		? QString::number(endpoint.port)
		: u"2398"_q;
	controller->show(Box<Ui::GenericBox>([=](
			not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(u"服务器地址"_q));

		const auto host = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			Ui::InputField::Mode::NoNewlines,
			rpl::single(u"服务器 IP 或域名"_q),
			endpoint.host));
		const auto port = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			Ui::InputField::Mode::NoNewlines,
			rpl::single(u"端口"_q),
			fallbackPort));

		const auto save = [=] {
			const auto hostValue = host->getLastText().trimmed();
			const auto portValue = port->getLastText().trimmed();
			bool portOk = false;
			const auto portNumber = portValue.toInt(&portOk);
			const auto hostInvalid = hostValue.isEmpty()
				|| hostValue.contains(u" "_q)
				|| hostValue.contains(u"\t"_q);
			if (hostInvalid) {
				host->showError();
				return;
			}
			if (!portOk || portNumber <= 0 || portNumber > 65535) {
				port->showError();
				return;
			}
			if (!SetCustomEndpoint(hostValue, portNumber)) {
				host->showError();
				return;
			}
			if (accountIndex >= 0) {
				SetAccountCustom(accountIndex);
			}
			box->closeBox();
			controller->showToast(accountIndex >= 0
				? u"服务器地址已保存；当前账号将使用自定义服务器，重启客户端后生效"_q
				: u"服务器地址已保存，重启客户端后生效"_q);
		};

		box->addButton(rpl::single(u"保存"_q), save);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
		box->setFocusCallback([=] { host->setFocusFast(); });
	}));
}

} // namespace CustomServers
