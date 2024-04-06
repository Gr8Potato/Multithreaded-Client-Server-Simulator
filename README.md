# Multithreaded-Client-Server-Simulator
This program simulates a multithreaded server processing jobs created in parallel. These processes are then added to a FIFO queue (implemented with a circular array) which the server then processes said requests by removing them from the queue. Their work is then logged to a file, and the work each thread reports the total jobs they've created and processed.

### Compilation-Execution
The GNU C Compiler can be used to compile this program. The following syntax is listed below.

`gcc MTCSS.c -o <exe_name> -lrt -lpthread`.

Once the program is ready for execution, the following command can be used to run the program:

`<executable_path> <client_thread_count> <server_thread_count> <job_pool_size> <total_jobs> <processing_time_ms>`

Once the program has completed you'll see the parameters you provided and the number of jobs each individual client/server thread has made/processed. Furthermore, `service.log` will have been created which can be accessed with the following command:
`cat service.log`

Each entry in `service.log` contains entries in the following format.

`<timestamp (in_nanoseconds)> <thread_type_(“client”_or_“server”)> <thread_id> <job_pool_entry_index> <job_id>.`

> [!CAUTION]
> I do not support people plagiarizing my code. I do not take responsibility for the unlawful actions of others.
