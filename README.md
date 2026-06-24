# wsn_server

This branch documents the `wsn_server`.

> `Scope:` This branch covers only the `controller`, `plugin`, and `MQTT` layers of the backend.

## Tech Stack

- `Frontend:` Vue 3
- `Backend:` Drogon (C++)
- `Database:` MariaDB

## Structure

- `Controllers` - HTTP route handlers, request/response logic
- `Plugins` - Drogon plugins (DB client)
- `MQTT` - subscriber/publisher integration for ingesting data from `wsn_gateway`

## Getting Started

```bash
# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./wsn_server
```

See the [Drogon documentation](https://github.com/drogonframework/drogon-docs) for full prerequisites (C++ compiler, CMake, Drogon itself, JsonCpp, etc.).

## Configuration

Database connection is configured via `config_example.json`. Copy it, fill in your values, and rename to `config.json` before running the server:

Edit the `db_clients` section in `config.json`:

```json
"name":                     "wsn",
"rdbms":                    "mysql",
"host":                     "127.0.0.1",
"port":                     3306,
"dbname":                   "wsn_db",
"user":                     "your_db_user",
"passwd":                   "your_db_password",
"is_fast":                  false,
"number_of_connections":    5,
"timeout":                  -1.0,
"auto_batch":               false
```

### What you can change

|           Field           |         Description         |
|---------------------------|-----------------------------|
| `host` / `port`           | MariaDB server address      |
| `user` / `passwd`         | DB credentials              |
| `number_of_connections`   | Connection pool size        |
| `timeout`                 | Query timeout in seconds    |
| `is_fast` / `auto_batch`  | Drogon DB client behavior - see [Drogon docs](https://github.com/drogonframework/drogon-docs) for details |

### What NOT to change

- **`name`** (`"wsn"`) and **`dbname`** (`"wsn_db"`) are hardcoded in the controllers' DB client lookups. Change them only if you also update every controller that references them.
## Notes

- `rdbms` stays `"mysql"` even for MariaDB - Drogon only recognizes `mysql`, `postgresql`, and `sqlite3`. MariaDB is wire-compatible with MySQL, so it uses the same driver.
- Using a different `rdbms`? You'll likely need to redesign the schema in `wsn_migration.sql`, since column types and syntax don't always port across engines.
- Currently deployed inside a WSL2 VM with port forwarding to the Windows host. Drogon itself supports other environments (e.g. Docker) - see the [Drogon docs](https://github.com/drogonframework/drogon-docs). Future versions may target one of these.