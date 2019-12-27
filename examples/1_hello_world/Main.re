/* Import the ReWeb module */
open ReWeb;

/* Define a service (i.e. function) to handle a "hello" request */
let hello = _ => "Hello, World!" |> Response.of_text |> Lwt.return;

/* Define a service to handle requests that do not have a specified route */
let notFound = _ => `Not_found |> Response.of_status |> Lwt.return;

/* Define your routes via simple pattern matching, note that we must be exhaustive */
let routes =
  fun
  | (`GET, ["hello"]) => hello
  | _ => notFound;

/* Run the server */
let () = routes |> Server.serve |> Lwt_main.run;
