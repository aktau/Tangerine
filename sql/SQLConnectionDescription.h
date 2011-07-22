#ifndef SQLCONNECTIONDESCRIPTION_H_
#define SQLCONNECTIONDESCRIPTION_H_

#include <QString>
#include <QFile>
#include <QDomDocument>
#include <QDebug>

#define DBD_DOCTYPE "thera-database"
#define DBD_ROOTTAG "connect"
#define DBD_LASTVERSION "1.0"

#define DBD_HOST "host"
#define DBD_PORT "port"
#define DBD_DBNAME "dbname"
#define DBD_USER "user"
#define DBD_PASS "password"

class SQLConnectionDescription {
	public:
		SQLConnectionDescription(const QString& file);
		SQLConnectionDescription(const QString& _host, int _port, const QString& _dbname, const QString& _user, const QString& _password);

		bool save(const QString& file) const;
		bool load(const QString& file);

		QString getHost() const { return host; }
		int getPort() const { return port; }
		QString getDbname() const { return dbname; }
		QString getUser() const { return user; }
		QString getPassword() const { return password; }

	private:
		QString host;
		int port;
		QString dbname;

		QString user;
		QString password;
};

SQLConnectionDescription::SQLConnectionDescription(const QString& file) {
	load(file);
}

SQLConnectionDescription::SQLConnectionDescription(const QString& _host, int _port, const QString& _dbname, const QString& _user, const QString& _password)
	: host(_host), port(_port), dbname(_dbname), user(_user), password(_password) { }

bool SQLConnectionDescription::save(const QString& filename) const {
	QFile file(filename);

	if (file.exists()) {
		qDebug() << "SQLConnectionDescription::save: Overwriting" << filename;
	}

	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);

		QDomDocument doc(DBD_DOCTYPE);
		QDomElement options = doc.createElement(DBD_ROOTTAG);
		options.setAttribute("version", DBD_LASTVERSION);

		{
			QDomElement option(doc.createElement(DBD_HOST));
			option.appendChild(doc.createTextNode(host));
			options.appendChild(option);
		}

		{
			QDomElement option(doc.createElement(DBD_PORT));
			option.appendChild(doc.createTextNode(QString::number(port)));
			options.appendChild(option);
		}

		{
			QDomElement option(doc.createElement(DBD_DBNAME));
			option.appendChild(doc.createTextNode(dbname));
			options.appendChild(option);
		}

		{
			QDomElement option(doc.createElement(DBD_USER));
			option.appendChild(doc.createTextNode(user));
			options.appendChild(option);
		}

		{
			QDomElement option(doc.createElement(DBD_PASS));
			option.appendChild(doc.createTextNode(password));
			options.appendChild(option);
		}

		doc.appendChild(options);

		doc.save(out, 1);
		file.close();
	}
	else {
		qDebug() << "SQLConnectionDescription::save: Could not open" << filename;

		return false;
	}

	return true;
}

bool SQLConnectionDescription::load(const QString& XMLFile) {
	QFile file(XMLFile);

	if (!file.exists()) {
		qDebug("SQLConnectionDescription::load: file %s did not exist", qPrintable(XMLFile));

		return false;
	}

	// open the file in read-only mode
	if (file.open(QIODevice::ReadOnly)) {
		QDomDocument doc;

		bool succes = doc.setContent(&file);

		file.close();

		if (succes) {
			QDomElement root(doc.documentElement());

			for (QDomElement option = root.firstChildElement(); !option.isNull(); option = option.nextSiblingElement()) {
				if (option.tagName() == DBD_HOST) host = option.text();
				if (option.tagName() == DBD_PORT) port = option.text().toInt();
				if (option.tagName() == DBD_DBNAME) dbname = option.text();
				if (option.tagName() == DBD_USER) user = option.text();
				if (option.tagName() == DBD_PASS) password = option.text();
			}
		}
		else {
			qDebug() << "SQLConnectionDescription::load: Reading XML file" << XMLFile << "failed";

			return false;
		}
	}
	else {
		qDebug() << "SQLConnectionDescription::load: Could not open"  << XMLFile;

		return false;
	}

	return true;
}

#endif /* SQLCONNECTIONDESCRIPTION_H_ */
