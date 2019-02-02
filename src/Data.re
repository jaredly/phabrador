
module Person = {
  type t = {userName: string, realName: string, phid: string, image: string};
  let parse = result => {
    module F = Lets.Opt;
    open Json.Infix;
    let%F phid = result |> Json.get("phid") |?> Json.string;
    let%F userName = result |> Json.get("userName") |?> Json.string;
    let%F realName = result |> Json.get("realName") |?> Json.string;
    let%F image = result |> Json.get("image") |?> Json.string;
    Some({userName, realName, image, phid});
  };
};

module Revision = {
  type t = {
    title: string,
    phid: string,
    id: int,
    repositoryPHID: string,
    authorPHID: string,
    summary: string,
    dateCreated: int,
    dateModified: int,
    status: string,
    color: string,
  };
  let parse = result => {
    module F = Lets.Opt;
    open Json.Infix;
    let%F phid = result |> Json.get("phid") |?> Json.string;
    let%F id = result |> Json.get("id") |?> Json.number |?>> int_of_float;
    let%F fields = result |> Json.get("fields");

    let%F title = fields |> Json.get("title") |?> Json.string;
    let%F repositoryPHID = fields |> Json.get("repositoryPHID") |?> Json.string;
    let%F authorPHID = fields |> Json.get("authorPHID") |?> Json.string;
    let%F summary = fields |> Json.get("summary") |?> Json.string;
    let%F dateCreated = fields |> Json.get("dateCreated") |?> Json.number |?>> int_of_float;
    let%F dateModified = fields |> Json.get("dateModified") |?> Json.number |?>> int_of_float;
    let%F statusObj = fields |> Json.get("status");
    let%F status = statusObj |> Json.get("value") |?> Json.string;
    let%F color = statusObj |> Json.get("color.ansi") |?> Json.string;
    print_endline(status);

    Some({title, phid, id, repositoryPHID, authorPHID, summary, dateModified, dateCreated, status, color});
  };
};