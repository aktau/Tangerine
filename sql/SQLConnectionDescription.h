#ifndef SQLCONNECTIONDESCRIPTION_H_
#define SQLCONNECTIONDESCRIPTION_H_

#include <QString>
#include <QFile>
#include <QDomDocument>
#include <QDebug>

#define DBD_DOCTYPE "thera-database"
#define DBD_ROOTTAG "connect"
#define DBD_LASTVERSION "1.0"

#define DBD_TYPE "type"
#define DBD_HOST "host"
#define DBD_PORT "port"
#define DBD_DBNAME "dbname"
#define DBD_USER "user"
#define DBD_PASS "password"

class SQLConnectionDescription {
	public:
		typedef enum {
			MYSQL,
			NUM_DB_TYPES
		} DbType;

	public:
		SQLConnectionDescription(const QString& file);
		SQLConnectionDescription(SQLConnectionDescription::DbType _type, const QString& _host, int _port, const QString& _dbname, const QString& _user, const QString& _password);

		static QString dbTypeToString(SQLConnectionDescription::DbType type);
		static SQLConnectionDescription::DbType dbStringToType(const QString& type);

		bool save(const QString& file) const;
		bool load(const QString& file);

		bool isValid() const { return mValid; }

		SQLConnectionDescription::DbType getType() const { return type; }
		QString getHost() const { return host; }
		int getPort() const { return port; }
		QString getDbname() const { return dbname; }
		QString getUser() const { return user; }
		QString getPassword() const { return password; }

	private:
		DbType type;

		QString host;
		int port;
		QString dbname;

		QString user;
		QString password;

		bool mValid;
};

SQLConnectionDescription::SQLConnectionDescription(const QString& file) : mValid(false) {
	load(file);
}

SQLConnectionDescription::SQLConnectionDescription(SQLConnectionDescription::DbType _type, const QString& _host, int _port, const QString& _dbname, const QString& _user, const QString& _password)
	: type(_type), host(_host), port(_port), dbname(_dbname), user(_user), password(_password), mValid(true) { }

QString SQLConnectionDescription::dbTypeToString(SQLConnectionDescription::DbType type) {
	switch (type) {
		case MYSQL:
			return "MySQL";

		default:
			return "UNKNOWN_DB_TYPE";
	}
}

SQLConnectionDescription::DbType SQLConnectionDescription::dbStringToType(const QString& type) {
	QString uType = type.toUpper();

	if (uType == "MYSQL") {
		return MYSQL;
	}
	else {
		return NUM_DB_TYPES;
	}
}

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
			QDomElement option(doc.createElement(DBD_TYPE));
			option.appendChild(doc.createTextNode(dbTypeToString(type)));
			options.appendChild(option);
		}

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
				if (option.tagName() == DBD_TYPE) type = dbStringToType(option.text());
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

	mValid = true;

	return true;
}

#endif /* SQLCONNECTIONDESCRIPTION_H_ */
