(** ReWeb - an ergonomic web framework. Based on the design proposed in
    {{: https://gist.github.com/yawaramin/f0a24f1b01b193dd6d251e5e43be65e9}} *)

module Make(Reqd : Request.REQD) = struct
  module Body = Body

  module Client = Client
  (** Make web requests. *)

  module Filter = Filter.Make(Reqd)
  (** Transform services. *)

  module Form = Form
  (** Decode web forms into specified types. Think of this like JSON
      decoding. *)

  module Headers = Httpaf.Headers

  module Request = Request.Make(Reqd)
  (** Read requests. *)

  module Response = Response
  (** Send responses. *)

  module Server = Server.Make(Reqd)
  (** Create and serve endpoints. *)
end

module Reqd = Httpaf.Reqd

