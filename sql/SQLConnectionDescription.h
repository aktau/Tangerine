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
			SQLITE,
			POSTGRESQL,
			NUM_DB_TYPES
		} DbType;

	public:
		SQLConnectionDescription(const QString& file);
		SQLConnectionDescription(SQLConnectionDescription::DbType _type, const QString& _host, int _port, const QString& _dbname, const QString& _user, const QString& _password);
		SQLConnectionDescription(SQLConnectionDescription::DbType _type, const QString& sqliteFile); // this is for an SQLite, specifying any other DbType will result in an isValid() being false

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
		QString getConnectionName() const { return (type == SQLITE) ? dbname : (host + ":" + QString::number(port) + "/" + dbname); }

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

SQLConnectionDescription::SQLConnectionDescription(SQLConnectionDescription::DbType _type, const QString& sqliteFile)
	: type(_type), dbname(sqliteFile), mValid(false) {
	if (type == SQLITE) mValid = true;
}

QString SQLConnectionDescription::dbTypeToString(SQLConnectionDescription::DbType type) {
	switch (type) {
		case MYSQL:
			return "MySQL";

		case POSTGRESQL:
			return "PostgreSQL";

		default:
			return "UNKNOWN_DB_TYPE";
	}
}

SQLConnectionDescription::DbType SQLConnectionDescription::dbStringToType(const QString& type) {
	QString uType = type.toUpper();

	if (uType == "MYSQL") {
		return MYSQL;
	}
	else if (uType == "POSTGRESQL") {
		return POSTGRESQL;
	}
	else {
		return NUM_DB_TYPES;
	}
}

bool SQLConnectionDescription::save(const QString& filename) const {
	assert(type != SQLITE);

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

bool SQLConnectionDescription::load(const QString& filename) {
	QFile file(filename);
	QFileInfo fileinfo(file);

	if (fileinfo.suffix() == "db") {
		// SQLite databases have the .db suffix, they are not XML files and the database name is
		// taken as equal to the filename (Qt convention)
		// it's not imporant whether the file exists or not, since it will be created if it doesn't
		// in the QSqlDatabase::open() call
		type = SQLITE;
		dbname = filename;

		return mValid = true;
	}

	if (!file.exists()) {
		qDebug("SQLConnectionDescription::load: file %s did not exist", qPrintable(filename));

		return mValid = false;
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
			qDebug() << "SQLConnectionDescription::load: Reading XML file" << filename << "failed";

			return mValid = false;
		}
	}
	else {
		qDebug() << "SQLConnectionDescription::load: Could not open"  << filename;

		return mValid = false;
	}

	return mValid = true;
}

#endif /* SQLCONNECTIONDESCRIPTION_H_ */
