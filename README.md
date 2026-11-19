# EmbeddedMarket
We are building Embedded Market to help people who build systems with microelectronics. By automatically extracting component specifications and constraints from datasheets, our software will allow users to build project BOMs and shop for components based on their pricing and compatibility, saving them both time and money.

# Run the prototype
Docker Desktop is the only requirement. From the repository root:

```sh
docker compose up --build
```

Open http://localhost:8080 once the containers are running. To return the database to its seed data, run `docker compose down -v`.

# Contributors
@AndrewDile : Andrew Dile <br>
@Dasgupta426 : Aakriti Dasgupta <br>
@IsaacEatsPythons: Isaac Kuang
