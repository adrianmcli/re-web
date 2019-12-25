module type REQD = sig
  type (_, _) t

  val request :  (_, _) t -> Httpaf.Request.t
  val request_body : (_, _) t -> [`read] Httpaf.Body.t

  val respond_with_bigstring :
    (_, _) t ->
    Httpaf.Response.t ->
    Bigstringaf.t ->
    unit

  val respond_with_streaming :
    ?flush_headers_immediately:bool ->
    (_, _) t ->
    Httpaf.Response.t ->
    [`write] Httpaf.Body.t
end

module type S = sig
  module Reqd : REQD

  type 'ctx t = {
    ctx : 'ctx;
    (** Any value. Can be changed by filters. Most useful if it's an
        object type so that filters can arbitrarily put named values of
        any type in the request-response pipeline. *)

    query : string;
    (** Either the query string (anything following the [?] in the path)
        or empty string. *)

    reqd : (Lwt_unix.file_descr, unit Lwt.t) Reqd.t;
  }

  val body : unit t -> Body.t
  (** [body request] gets the [request] body. There is a chance that the
      body may already have been read, in which case trying to read it
      again will error. However in a normal request pipeline as bodies
      are read by filters, that should be minimized. *)

  val body_string : ?buf_size:int -> unit t -> string Lwt.t
  (** [body_string ?buf_size request] returns the request body converted
      into a string, internally using a buffer of size [buf_size] with a
      default of Lwt's default buffer size. *)

  val context : 'ctx t -> 'ctx

  val header : string -> _ t -> string option
  (** [header name request] gets the last value corresponding to the
      given header, if present. *)

  val headers : string -> _ t -> string list
  (** [headers name request] gets all the values corresponding to the
      given header. *)

  val make :
    string ->
    (Lwt_unix.file_descr, unit Lwt.t) Reqd.t ->
    unit t
  (** [make query reqd] returns a new request containing the given
      [query] and Httpaf [reqd]. *)

  val query : _ t -> string
  (** [query request] gets the query string of the [request]. *)
end

module Make(Reqd: REQD) : S

