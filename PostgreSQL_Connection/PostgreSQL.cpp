//**#**
#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <tuple>

using namespace std;

int main() {
	try {
		pqxx::connection conn("dbname=fps_game_db user=fps_user password=3567 host=localhost port=5432");

		if (!conn.is_open()) {
			cerr << "Failed to connect to PostgreSQL Database!\n";
			return 1;
		}

		cout << "Connected to PostgreSQL Database!\n";

		vector<tuple<string, string, string>> users;

		{
			pqxx::work txn(conn);
			pqxx::result result = txn.exec("SELECT username, email, id FROM Users");
			txn.commit();

			for (const auto& row : result) {
				string username = row["username"].c_str();
				string email = row["email"].c_str();
				string id = row["id"].c_str();

				users.emplace_back(username, email, id);
			}
		}

		cout << "User List:\n";
		for (const auto& [username, email, id] : users) {
			cout << "Username: " << username
				<< " | Email: " << email
				<< " | ID: " << id << endl;
		}

		try {
			conn.close();
		}
		catch (const std::exception& e) {
			cerr << "Warning on close: " << e.what() << endl;
	}

	catch (const exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}

	return 0;
}
//**#**