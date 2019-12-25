open ReWeb;

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

let hello = _ => "Hello, World!" |> Response.of_text |> Lwt.return;

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

let getLogin = _ =>
  View.login(~rememberMe=true) |> Response.of_view |> Lwt.return;

let postLogin = request => {
  let {User.username, password} = Request.context(request)#form;

  password
  |> Printf.sprintf(
       {|<h1>Logged in</h1>
<p>with username = "%s", password = "%s"</p>|},
       username,
     )
  |> Response.of_html
  |> Lwt.return;
};

let getStatic = (fileName, _) =>
  fileName
  |> String.concat("/")
  |> (++)("/")
  |> Response.of_file(~content_type="text/plain");

let echoBody = request =>
  request
  |> Request.body
  |> Response.make(
       ~status=`OK,
       ~headers=
         Headers.of_list([
           ("content-type", "application/octet-stream"),
           ("connection", "close"),
         ]),
     )
  |> Lwt.return;

let exclaimBody = request =>
  request
  |> Request.body_string
  |> Lwt.map(string => Response.of_text(string ++ "!"));

let internalServerError = string =>
  string |> Response.of_text(~status=`Internal_server_error) |> Lwt.return;

/** [getTodo(id, request)] gets the todo item with ID [id] from the JSON
    Placeholder API, and extracts and returns only the title of the todo
    item. */
let getTodo = (id, _) => {
  let%lwt response =
    Client.Once.get("https://jsonplaceholder.typicode.com/todos/" ++ id);

  switch (response) {
  | Ok(response) =>
    let%lwt json = Body.to_json(response.Response.body);

    /* We are manually pattern-matching against the JSON body here.
       In the future we'll use a JSON decoder shipped with ReWeb to do
       that. */
    switch (json) {
    | Ok(`O(props)) =>
      Lwt.return(
        switch (List.assoc("title", props)) {
        | `String(title) => Response.of_text(title)
        | _
        | exception Not_found =>
          Response.of_text(
            ~status=`Internal_server_error,
            "JSON response malformed",
          )
        },
      )
    | _ => internalServerError("getTodo: malformed JSON response")
    };
  | Error(string) => internalServerError(string)
  };
};

let authHello = request => {
  let context = Request.context(request);

  context#password
  |> Printf.sprintf("Username = %s\nPassword = %s", context#username)
  |> Response.of_text
  |> Lwt.return;
};

let authServer =
  fun
  | (`GET, ["hello"]) => authHello
  | _ => notFound;

let msie = Str.regexp(".*MSIE.*");

let rejectExplorer = (next, request) =>
  switch (Request.header("user-agent", request)) {
  | Some(ua) when Str.string_match(msie, ua, 0) =>
    "Please upgrade your browser"
    |> Response.of_text(~status=`Unauthorized)
    |> Lwt.return
  | _ => next(request)
  };

let server =
  fun
  | (`GET, ["hello"]) => hello
  | (`GET, ["header", name]) => getHeader(name)
  | (`GET, ["login"]) => getLogin
  | (`POST, ["login"]) => Filter.body_form(User.form) @@ postLogin
  | (`GET, ["static", ...fileName]) => getStatic(fileName)
  | (`POST, ["body"]) => echoBody
  | (`POST, ["body-bang"]) => exclaimBody
  | (`POST, ["json"]) => Filter.body_json @@ hello
  | (`GET, ["todos", id]) => getTodo(id)
  | (meth, ["auth", ...path]) =>
    Filter.basic_auth @@ authServer @@ (meth, path)
  | _ => notFound;

let server = route => rejectExplorer @@ server @@ route;
let () = server |> Server.serve |> Lwt_main.run;
