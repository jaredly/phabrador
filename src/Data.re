
module Person = {
  type t = {
    userName: string,
    realName: string,
    phid: string,
    image: string,
    loadedImage: option(FluidMac.Fluid.NativeInterface.image),
  };
  let parse = result => {
    module F = Lets.Opt;
    open Json.Infix;
    let%F phid = result |> Json.get("phid") |?> Json.string;
    let%F userName = result |> Json.get("userName") |?> Json.string;
    let%F realName = result |> Json.get("realName") |?> Json.string;
    let%F image = result |> Json.get("image") |?> Json.string;
    Some({userName, realName, image, phid, loadedImage: None});
  };
};

module Repository = {
  type t = {
    phid: string,
    name: string,
    callsign: string,
  };
  let parse = result => {
    module F = Lets.Opt;
    open Json.Infix;
    let%F phid = result |> Json.get("phid") |?> Json.string;
    let%F fields = result |> Json.get("fields");
    let%F name = fields |> Json.get("name") |?> Json.string;
    let%F callsign = fields |> Json.get("callsign") |?> Json.string;
    Some({phid, name, callsign})
  };
};

module Diff = {
  type t = {
    id: int,
    phid: string,
    revisionPHID: string,
    authorPHID: string,
    repositoryPHID: string,
    branch: option(string),
    dateCreated: int,
    dateModified: int,
  };
};

module Revision = {
  type t = {
    title: string,
    phid: string,
    id: int,
    repositoryPHID: string,
    authorPHID: string,
    diffPHID: string,
    summary: string,
    dateCreated: int,
    dateModified: int,
    snoozed: bool,
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
    let%F diffPHID = fields |> Json.get("diffPHID") |?> Json.string;
    let%F summary = fields |> Json.get("summary") |?> Json.string;
    let%F dateCreated = fields |> Json.get("dateCreated") |?> Json.number |?>> int_of_float;
    let%F dateModified = fields |> Json.get("dateModified") |?> Json.number |?>> int_of_float;
    let%F statusObj = fields |> Json.get("status");
    let%F status = statusObj |> Json.get("value") |?> Json.string;
    let%F color = statusObj |> Json.get("color.ansi") |?> Json.string;

    Some({
      title,
      phid,
      id,
      snoozed: false,
      repositoryPHID,
      authorPHID,
      diffPHID,
      summary,
      dateModified,
      dateCreated,
      status,
      color,
    });
  };

  type groups = {
    accepted: list(t),
    needsReview: list(t),
    needsRevision: list(t),
    changesPlanned: list(t),
  };
  type all = {
    mine: groups,
    theirs: groups,
  };

  let checkStatus = (status, r) => r.status == status;
  let makeGroups = (revisions: list(t)) => {
    accepted: revisions->Belt.List.keep(checkStatus("accepted")),
    needsReview: revisions->Belt.List.keep(checkStatus("needs-review")),
    needsRevision: revisions->Belt.List.keep(checkStatus("needs-revision")),
    changesPlanned: revisions->Belt.List.keep(checkStatus("changes-planned")),
  };
  let appendGroups = (a, b) => {
    accepted: List.append(a.accepted, b.accepted),
    needsReview: List.append(a.accepted, b.accepted),
    needsRevision: List.append(a.accepted, b.accepted),
    changesPlanned: List.append(a.accepted, b.accepted),
  };
  let append = (a, b) => {
    mine: appendGroups(a.mine, b.mine),
    theirs: appendGroups(a.theirs, b.theirs),
  };
  let organize = (person: Person.t, revisions: list(t)) => {
    mine: makeGroups(revisions->Belt.List.keep(r => r.authorPHID == person.phid)),
    theirs: makeGroups(revisions->Belt.List.keep(r => r.authorPHID != person.phid))
  };
  let mapGroups = ({accepted, needsReview, needsRevision, changesPlanned}, m) => {
    accepted: m(accepted),
    needsReview: m(needsReview),
    needsRevision: m(needsRevision),
    changesPlanned: m(changesPlanned),
  };
  let map = ({mine, theirs}, m) => {
    mine: mapGroups(mine, m),
    theirs: mapGroups(theirs, m),
  };
};
