/* This is an example that shows the various features of ReWeb for
   creating web servers. You can skip to the bottom of the file to see
   the router, and work your way back up if you want. */

/* [ReWeb] contains just a handful of modules so there's very little
   chance of a conflict. */
open ReWeb;

/** [notFound(request)] is a service that responds with a formatted HTML
    404 Not Found message. */
let notFound = _ =>
  {|<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Not Found</title>
  </head>
  <body>
    <h1>Not Found</h1>
  </body>
</html>|}
  |> Response.of_html(~status=`Not_found)
  |> Lwt.return;

/** [hello(request)] is a service that just responds with a hello-world
    message. */
let hello = _ => "Hello, World!" |> Response.of_text |> Lwt.return;

/** [getHeader(name, request)] is a service that returns the contents of
    the [request] header named [name]. */
let getHeader = (name, request) =>
  switch (Request.header(name, request)) {
  | Some(value) =>
    value
    |> Printf.sprintf({|<h1>GET /header/%s</h1>
<p>%s</p>|}, name)
    |> Response.of_html
    |> Lwt.return
  | None => notFound(request)
  };

/** [getLogin(request)] is a service that renders a view. Open the file
    [View.re] to see what the [login] view looks like. */
let getLogin = _ =>
  View.login(~rememberMe=true) |> Response.of_view |> Lwt.return;

// Helper to respond with credentials in response body.
let respondCredentials = (username, password) =>
  password
  |> Printf.sprintf(
       {|<h1>Logged in</h1>
<p>with username = "%s", password = "%s"</p>|},
       username,
     )
  |> Response.of_html
  |> Lwt.return;

/** [postLogin(request)] is a service that handles the [POST /login]
    endpoint. It's statically guaranteed access to the request body as a
    decoded, strongly-typed form, because a form decoder filter was
    added before the service in the router.

    It returns the credentials in the response body. */
let postLogin = request => {
  let {User.username, password} = Request.context(request)#form;
  respondCredentials(username, password);
};

/** [getLoginQuery(request)] is a service that handles the
    [GET /login-query] endpoint. It's statically guaranteed access to
    the decoded, strongly-typed request query, because a query decoder
    filter was added before the service in the router. */
let getLoginQuery = request => {
  let {User.username, password} = Request.context(request)#query;
  respondCredentials(username, password);
};

/** [getStatic(fileName, request)] is a service that returns the
    contents of [fileName] (if found). */
let getStatic = (fileName, _) =>
  fileName
  |> String.concat("/")
  |> (++)("/")
  |> Response.of_file(~content_type="text/plain");

/** [echoBody(request)] is a service that directly echoes the [request]
    body back to the client, without touching it at all. */
let echoBody = request =>
  request
  |> Request.body
  |> Response.make(
       ~status=`OK,
       ~headers=[
         ("content-type", "application/octet-stream"),
         ("connection", "close"),
       ],
     )
  |> Lwt.return;

/** [exclaimBody(request)] is a service that echoes the [request] body
    but with an exclamation mark added to the end. */
let exclaimBody = request =>
  request
  |> Request.body_string
  |> Lwt.map(string => Response.of_text(string ++ "!"));

// Helper function (not a service as it doesn't take a request)
let internalServerError = message =>
  `Internal_server_error |> Response.of_status(~message) |> Lwt.return;

/** [getTodo(id, request)] gets the todo item with ID [id] from the JSON
    Placeholder API and returns the response {i exactly,} including all
    headers from the source. You can test this by running the command:
    [curl -i localhost:8080/todos/1] and checking the headers. */
let getTodo = (id, _) => {
  let%lwt response =
    Client.New.get("https://jsonplaceholder.typicode.com/todos/" ++ id);

  switch (response) {
  | Ok(response) => Lwt.return(response)
  | Error(string) => internalServerError(string)
  };
};

/** [getTodoTitle(id, request)] gets the todo item with ID [id] from the
    JSON Placeholder API, and extracts and returns only the title of the
    todo item. */
let getTodoTitle = (id, request) => {
  let%lwt response = getTodo(id, request);
  let%lwt json = response |> Response.body |> Body.to_json;

  /* We are manually pattern-matching against the JSON body here.
     In the future we'll use a JSON decoder shipped with ReWeb to do
     that. */
  switch (json) {
  | Ok(`O(props)) =>
    switch (List.assoc("title", props)) {
    | `String(title) => title |> Response.of_text |> Lwt.return
    | _
    | exception Not_found =>
      internalServerError("getTodo: malformed JSON response")
    }
  | _ => internalServerError("getTodo: malformed JSON response")
  };
};

/** [authHello(request)] is a service that handles [GET /auth/hello].
    It's statically guaranteed to access the credentials in the request
    context (because the filter was applied in the top-level server). */
let authHello = request => {
  let context = Request.context(request);

  context#password
  |> Printf.sprintf("Username = %s\nPassword = %s", context#username)
  |> Response.of_text
  |> Lwt.return;
};

// Server for /auth/... endpoints, enforcing basic auth (see below)
let authServer =
  fun
  | (`GET, ["hello"]) => authHello
  | _ => notFound;

let msie = Str.regexp(".*MSIE.*");

// Filter that rejects requests from MSIE
let rejectExplorer = (next, request) =>
  switch (Request.header("user-agent", request)) {
  | Some(ua) when Str.string_match(msie, ua, 0) =>
    `Unauthorized
    |> Response.of_status(~message="Please upgrade your browser")
    |> Lwt.return
  | _ => next(request)
  };

/* The top-level server (which is also a router simply by using pattern-
   matching syntax). In the filter examples below which use [@@] you can
   think of it as 'and' or 'then', i.e. 'first apply this filter then
   send the request to the service'. Actually [@@] is a generic operator
   provided by the standard library: [f(x, y) == f(x) @@ y]. */
let server =
  fun
  | (`GET, ["hello"]) => hello
  | (`GET, ["header", name]) => getHeader(name)
  | (`GET, ["login"]) => getLogin
  /* Applies a filter to the [POST /login] endoint to decode a form in
     the request body to a strongly-typed value. Returns 400 Bad Request
     if form decoding fails. Open the file [User.re] to see how the form
     is defined. */
  | (`POST, ["login"]) => Filter.body_form(User.form) @@ postLogin
  /* Applies a filter to the [GET /login-query] endpoint to decode the
     request query to the same form as above. For demo purposes only,
     obviously we won't be sending login credentials in the query in
     real code :-) */
  | (`GET, ["login-query"]) => Filter.query_form(User.form) @@ getLoginQuery
  | (`GET, ["static", ...fileName]) => getStatic(fileName)
  | (`POST, ["body"]) => echoBody
  | (`POST, ["body-bang"]) => exclaimBody
  /* Applies a filter to the [POST /json] endpoint to parse the request
     body as JSON. Returns 400 Bad Request if JSON parsing fails. */
  | (`POST, ["json"]) => Filter.body_json @@ hello
  | (`GET, ["todos", id]) => getTodo(id)
  | (`GET, ["todos", "titles", id]) => getTodoTitle(id)
  // Route to a 'nested' server, and also apply a filter to this scope
  | (meth, ["auth", ...path]) =>
    Filter.basic_auth @@ authServer @@ (meth, path)
  /* Example of putting the service directly in the router, usually we
     avoid this because we'd like to keep the router small and readable. */
  | (`GET, ["redirect"]) => (
      _ => "/hello" |> Response.of_redirect |> Lwt.return
    )
  | _ => notFound;

// Apply a top-level filter to the entire server
let server = route => rejectExplorer @@ server @@ route;

// Run the server
let () = server |> Server.serve |> Lwt_main.run;
