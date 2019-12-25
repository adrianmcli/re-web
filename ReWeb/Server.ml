module H = Httpaf

module Make(Reqd : Request.REQD) = struct
  module Request = Request.Make(Reqd)

  type path = string list
  type route = H.Method.t * path
  type 'ctx service = 'ctx Request.t -> Response.t Lwt.t
  type 'ctx t = route -> 'ctx service

  let segment path = path |> String.split_on_char '/' |> List.tl

  let parse_route { H.Request.meth; target; _ } =
    Printf.printf "ReWeb.Server: %s %s" (H.Method.to_string meth) target;

    match String.split_on_char '?' target with
    | [path; query] -> meth, segment path, query
    | [path] -> meth, segment path, ""
    | _ -> failwith "ReWeb.Server: failed to parse route"

  let string_of_unix_addr = function
    | Unix.ADDR_UNIX string -> string
    | Unix.ADDR_INET (inet_addr, _) -> Unix.string_of_inet_addr inet_addr

  let schedule_chunk writer { H.IOVec.off; len; buffer } =
    H.Body.schedule_bigstring writer ~off ~len buffer

  let to_stream body = body
    |> Piaf.Body.to_stream
    |> Lwt_stream.map Body.make_chunk

  let error_handler _client_addr ?(request:_) _error _start_resp =
    failwith "!"

  let serve ?(port=8080) server =
    let request_handler client_addr reqd =
      let send ?(log=true) { Response.envelope; body } =
        if log then begin
          let code = H.Status.to_code envelope.H.Response.status in
          client_addr
          |> string_of_unix_addr
          |> Printf.printf " %d %s\n%!" code
        end;

        let send stream =
          let writer = Reqd.respond_with_streaming reqd envelope in
          let fully_written =
            Lwt_stream.iter (schedule_chunk writer) stream
          in
          Lwt.on_success fully_written @@ fun _ ->
            H.Body.close_writer writer
        in
        match body with
        | Body.Single bigstring ->
          Reqd.respond_with_bigstring reqd envelope bigstring
        | Body.Multi stream -> send stream
        | Body.Piaf body -> body |> to_stream |> send
      in
      let meth, path, query = reqd |> Reqd.request |> parse_route in
      let response = reqd |> Request.make query |> server (meth, path) in
      Lwt.on_success response send
    in
    let conn_handler = Httpaf_lwt_unix.Server.create_connection_handler
      ~request_handler
      ~error_handler
    in
    let listen_addr = Unix.(ADDR_INET (inet_addr_loopback, port)) in
    let open Lwt_let in
    let* lwt_server =
      Lwt_io.establish_server_with_client_socket listen_addr conn_handler
    in
    let* () = Lwt_io.printf "ReWeb.Server: listening on port %d\n" port in
    let forever, _ = Lwt.wait () in
    forever
end

