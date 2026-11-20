/*
This file is part of a custom tdesktop build.

Helpers for running an official account and a self-hosted (custom) account
in the same client, plus the Settings switches that enable each server.
*/
#pragma once

#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

namespace CustomServers {

[[nodiscard]] inline QString SettingsFile() {
	return cWorkingDir() + u"tdata/custom_servers.ini"_q;
}

[[nodiscard]] inline bool OfficialEnabled() {
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	return settings.value(u"official_server"_q, true).toBool();
}

inline void SetOfficialEnabled(bool value) {
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	settings.setValue(u"official_server"_q, value);
}

[[nodiscard]] inline bool SelfHostedEnabled() {
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	return settings.value(u"self_hosted_server"_q, true).toBool();
}

inline void SetSelfHostedEnabled(bool value) {
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	settings.setValue(u"self_hosted_server"_q, value);
}

// Account slots that use the self-hosted server. The environment variable
// takes precedence; otherwise the value is read from custom_servers.ini so
// launching Telegram.exe directly (without the launcher scripts) still works.
[[nodiscard]] inline QString AccountSpec() {
	if (const auto env = qEnvironmentVariable("TDESKTOP_DC_ACCOUNT");
		!env.isEmpty()) {
		return env;
	}
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	return settings.value(u"accounts"_q).toString();
}

// Which account slots use the self-hosted server. Supports "0", "1+"
// (every slot starting from 1) and comma separated lists; empty means every
// account, matching the old build behaviour.
[[nodiscard]] inline bool MatchesAccount(int index) {
	const auto spec = AccountSpec();
	if (spec.isEmpty()) {
		return true;
	}
	const auto parts = spec.split(u","_q, Qt::SkipEmptyParts);
	for (const auto &part : parts) {
		const auto item = part.trimmed();
		if (item.isEmpty()) {
			continue;
		}
		if (item.endsWith(u"+"_q)) {
			if (index >= item.left(item.size() - 1).toInt()) {
				return true;
			}
		} else if (item.toInt() == index) {
			return true;
		}
	}
	return false;
}

// Default files used when the launcher scripts did not set the environment.
// They can be overridden through custom_servers.ini. A copy inside tdata keeps
// the directly launched exe portable: the folder can be moved without the
// endpoint/key paths pointing back into the build tree.
[[nodiscard]] inline QString StoredEndpointsFile() {
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	return settings.value(u"endpoints_file"_q).toString();
}

[[nodiscard]] inline QString DefaultEndpointsFile() {
	if (const auto stored = StoredEndpointsFile(); !stored.isEmpty()) {
		return stored;
	}
	const auto local = cWorkingDir() + u"tdata/endpoints.txt"_q;
	if (QFileInfo::exists(local)) {
		return local;
	}
	return u"E:\\game\\Telegram\\server\\owpengram-232\\data\\endpoints.txt"_q;
}

[[nodiscard]] inline QString DefaultRsaKeyFile() {
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	if (const auto stored = settings.value(u"rsa_key_file"_q).toString();
		!stored.isEmpty()) {
		return stored;
	}
	const auto local = cWorkingDir() + u"tdata/server_rsa.pub.pem"_q;
	if (QFileInfo::exists(local)) {
		return local;
	}
	return u"E:\\game\\Telegram\\server\\owpengram-232\\data\\server_rsa.pub.pem"_q;
}

struct CustomEndpoint {
	QString host;
	int port = 0;
};

[[nodiscard]] inline CustomEndpoint ReadCustomEndpoint() {
	QFile file(DefaultEndpointsFile());
	if (!file.open(QIODevice::ReadOnly)) {
		return {};
	}
	QTextStream in(&file);
	while (!in.atEnd()) {
		const auto line = in.readLine().trimmed();
		if (line.isEmpty() || line.startsWith(u"#"_q)) {
			continue;
		}
		const auto parts = line.split(
			QRegularExpression(u"\\s+"_q),
			Qt::SkipEmptyParts);
		if (parts.size() >= 3 && parts[0].toInt() == 2) {
			return { parts[1], parts[2].toInt() };
		}
	}
	return {};
}

[[nodiscard]] inline QString CustomEndpointAddress() {
	const auto endpoint = ReadCustomEndpoint();
	if (endpoint.host.isEmpty() || endpoint.port <= 0) {
		return u"未设置"_q;
	}
	return endpoint.host + u":"_q + QString::number(endpoint.port);
}

inline bool SetCustomEndpoint(const QString &host, int port) {
	if (host.isEmpty() || port <= 0 || port > 65535) {
		return false;
	}
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	const auto path = cWorkingDir() + u"tdata/custom_endpoints.txt"_q;
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	QTextStream out(&file);
	out << "2 " << host << " " << port << "\n";

	QSettings settings(SettingsFile(), QSettings::IniFormat);
	settings.setValue(u"endpoints_file"_q, path);
	settings.setValue(u"self_hosted_server"_q, true);
	settings.sync();
	return true;
}

// Used by the login-page address dialog: setting an address there should also
// make the account being added a self-hosted one, instead of leaving the slot
// on the official servers until the operator edits custom_servers.ini.
inline bool SetAccountCustom(int index) {
	if (index < 0) {
		return false;
	}
	QSettings settings(SettingsFile(), QSettings::IniFormat);
	const auto current = settings.value(u"accounts"_q).toString();
	if (current.isEmpty()) {
		return true;
	}
	if (MatchesAccount(index)) {
		return true;
	}
	settings.setValue(
		u"accounts"_q,
		current + u","_q + QString::number(index));
	settings.sync();
	return true;
}

// Endpoints file that points every DC at a closed local port, so a disabled
// server gets no traffic at all instead of falling back to official servers.
[[nodiscard]] inline QString BlockedEndpointsFile() {
	const auto path = cWorkingDir() + u"tdata/custom_servers_blocked.txt"_q;
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return QString();
	}
	QTextStream out(&file);
	for (int dc = 1; dc != 6; ++dc) {
		out << dc << " 127.0.0.1 1\n";
	}
	return path;
}

} // namespace CustomServers
