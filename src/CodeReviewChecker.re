
open FluidMac;
open Data;

open Fluid.Native;

module Api = Api;

let str = Fluid.string;

let gray = n => {r: n, g: n, b: n, a: 1.};

external openUrl: string => unit = "codeReviewChecker_openUrl";

module TimeoutTracker = FluidMac.Tracker({type arg = unit; let name = "codeReviewChecker_timeout_cb"; let once = true; type res = unit});
external setTimeout: (TimeoutTracker.callbackId, int) => unit = "codeReviewChecker_setTimeout";
let setTimeout = (fn, time) => setTimeout(TimeoutTracker.track(fn), time);

let timePrinter = ODate.Unix.To.generate_printer("%I:%M%P")->Lets.Opt.force;
let datePrinter = ODate.Unix.To.generate_printer("%b %E, %Y")->Lets.Opt.force;

let recentDate = seconds => {
  open ODate.Unix;
  let date = From.seconds(seconds);
  let now = now();
  let today = beginning_of_the_day(~tz=ODate.Local, now);
  let yesterday = today->advance_by_days(-1);

  if (date > today) {
    "Today at " ++ To.string(~tz=ODate.Local, timePrinter, date)
  } else if (date > yesterday) {
    "Yesterday at " ++ To.string(~tz=ODate.Local, timePrinter, date)
  } else {
    let diff = between(date, now);
    let days = ODuration.To.day(diff);
    if (days <= 10) {
      Printf.sprintf("%d days ago", days);
    } else if (days <= 7 * 4) {
      Printf.sprintf("%d weeks ago", days / 7);
    } else {
      To.string(~tz=ODate.Local, datePrinter, date)
    }
  }
};

let%component revision = (~rev: Data.Revision.t, ~snoozeItem, hooks) => {
  let author = rev.author;
  let repo = rev.repository;
  if (rev.snoozed) {
    <view layout={Layout.style(
      ~paddingVertical=8.,
      ~marginHorizontal=8.,
      ~alignSelf=AlignStretch,
      ~flexDirection=Row,
      ())}
    >
      {str(~layout=Layout.style(~flexGrow=1., ()), ~font={fontName: "Helvetica", fontSize: 12.}, rev.Revision.title)}
      <button
        onPress={() => {
          if (rev.snoozed) {
            snoozeItem(rev.phid, None)
          } else {
            let tomorrow = ODate.Unix.From.seconds_float(Unix.time())
            -> ODate.Unix.beginning_of_the_day(~tz=ODate.Local, _)
            -> ODate.Unix.advance_by_days(1);
            snoozeItem(rev.phid, Some(tomorrow->ODate.Unix.To.seconds_float))
          }
        }}
        title={"Unsnooze"}
      />
    </view>
  } else {

  <view layout={Layout.style(
    ~paddingVertical=8.,
    ~marginHorizontal=8.,
    ~flexDirection=Row,
    ~alignSelf=AlignStretch,
    ())}
  >
    <view
    >
      {switch author {
        | None => str("Unknown author: " ++ rev.authorPHID)
        | Some(person) =>
        <view>
          /* {str(person.userName)} */
          /* <loadingImage src={person.image} 
          layout={Layout.style(~width=30., ~height=30., ())} /> */
          <image
            src={
              switch (person.loadedImage) {
                | None => Plain(person.image)
                | Some(i) => Preloaded(i)
              }
            }
            layout={Layout.style(~margin=3., ~width=30., ~height=30., ())}
          />
        <button
          layout={Layout.style(~left=-5., ())}
          onPress={() => openUrl(Api.diffUrl(rev.id))}
          title="Go"
        />
        </view>
      }}
    </view>
    <view layout={Layout.style(~flexGrow=1., ~flexShrink=1., ())} >
      <view layout={Layout.style(~flexDirection=Row, ())}>
        {str(~layout=Layout.style(~flexGrow=1., ~flexShrink=1., ()), ~font={fontName: "Helvetica", fontSize: 18.}, rev.Revision.title)}
        <button
          onPress={() => {
            if (rev.snoozed) {
              snoozeItem(rev.phid, None)
            } else {
              let tomorrow = ODate.Unix.From.seconds_float(Unix.time())
              -> ODate.Unix.beginning_of_the_day
              -> ODate.Unix.advance_by_days(1);
              snoozeItem(rev.phid, Some(tomorrow->ODate.Unix.To.seconds_float))
            }
          }}
          title={rev.snoozed ? "❗" : "💤"}
        />
      </view>
      <view layout={Layout.style(~flexDirection=Row, ())}>
        {str(recentDate(rev.dateModified))}
        <view layout={Layout.style(~flexGrow=1., ())} />
        {str(switch repo {
          | None => "Unknown repo"
          | Some({name}) =>
          switch (rev.diff) {
            | Some({branch: Some(branch)}) => branch ++ " : "
            | _ => ""
          } ++ name
        })}
      </view>
    </view>
  </view>
  }
};

let%component revisionList = (
  ~snoozeItem,
  ~revisions: list(Revision.t),
  ~title,
  hooks) => {
  if (revisions == []) {
    Fluid.Null
  } else {
    <view layout={Layout.style(~alignItems=AlignStretch, ())}>
      <view backgroundColor=gray(0.9) layout={Layout.style(~paddingHorizontal=8., ~paddingVertical=4., ())}>
        {str(title)}
      </view>
      {Fluid.Native.view(
        ~children=revisions->Belt.List.sort((a, b) => b.dateModified - a.dateModified)->Belt.List.map(rev => <revision snoozeItem rev /> ),
        ()
      )}
    </view>
  }
};

let fetchData = () => {
  module C = Lets.Async.Result;
  let%C person = Api.whoAmI;
  let%C revisions = Api.getRevisions(person);
  let users = Api.getUsers(revisions->Belt.List.map(r => r.Revision.authorPHID));
  let repos = Api.getRepositories(revisions->Belt.List.map(r => r.repositoryPHID));
  let diffs = Api.getDiffs(revisions->Belt.List.map(r => r.diffPHID));
  let%C users = users;
  let%C repos = repos;
  let%C diffs = diffs;
  let revisions = revisions->Belt.List.map(r => {
    ...r,
    author: users->Belt.Map.String.get(r.authorPHID),
    repository: repos->Belt.Map.String.get(r.repositoryPHID),
    diff: diffs->Belt.Map.String.get(r.diffPHID),
  })
  let revisions = Revision.organize(person, revisions);
  C.resolve((person, revisions));
};

let makeTitle = (revisions: Revision.all) => {
  let items = [
    (revisions.mine.accepted, "✅"),
    (revisions.mine.needsRevision, "❌"),
    (revisions.theirs.needsReview, "🙏"),
  ]
  ->Belt.List.keepMap(((items, emoji)) => {
    let items = items->Belt.List.keep(r => !r.snoozed);
    items === []
      ? None
      : Some(
          emoji ++ " " ++ string_of_int(List.length(items)),
        )
  });

  if (items === []) {
    "🐶"
  } else {
    items |> String.concat(" · ");
  }
};

let setCancellableTimeout = (fn, tm) => {
  let cancelled = ref(false);
  setTimeout(() => {
    if (!cancelled^) {
      fn();
    }
  }, tm);
  () => cancelled := true;
};

let refreshTime = 5 * 60 * 1000;
// let refreshTime = 3 * 1000;

let updateSnoozed = revisions =>  {
  let now = Unix.time();
  revisions->Revision.map(Belt.List.map(_, Config.setSnoozed(Config.current^, now)));
};

Printexc.record_backtrace(true);

let%component main = (~assetsDir, ~refresh, ~setTitle, hooks) => {
  let%hook (data, setData) = Fluid.Hooks.useState(None);
  let%hook (refreshing, setRefreshing) = Fluid.Hooks.useState(false);

  let snoozeItem = (phid, until) => {
    print_endline("Snoozing " ++ phid);
    // Gc.full_major();
    switch (data) {
      | None => ()
      | Some((p, r)) =>
        Config.toggleSnoozed(phid, until);
        let revisions = updateSnoozed(r);
        setTitle(Fluid.App.String(makeTitle(revisions)));
        setData(Some((p, revisions)))
    };
  };
  /* print_endline("render"); */

  let%hook () =
    Fluid.Hooks.useEffect(
      () => {
        let cancel = ref(() => ());
        let rec loop = () => {
          cancel^();
          setRefreshing(true);
          let%Lets.Async.Consume result = fetchData();
          /* print_endline("Fetched"); */
          switch result {
            | Error(error) =>
              setTitle(String("⌛"));
              setData(None);
            | Ok((person, revisions)) =>
              let revisions = updateSnoozed(revisions);
              setTitle(Fluid.App.String(makeTitle(revisions)));
              /* print_endline("a"); */
              setData(Some((person, revisions)));
          };
          /* print_endline("b"); */
          setRefreshing(false);
          /* print_endline("c"); */
          cancel := setCancellableTimeout(loop, refreshTime);
          /* print_endline("Updated"); */
        };
        refresh := loop;
        loop();

        () => ();
      },
      (),
    );

  <view
    layout={Layout.style(
      ~width=500.,
      ~maxHeight=800.,
      /* ~height=500., */
      ()
    )}
  >
    {str(~layout=Layout.style(~position=Absolute, ~top=5., ~right=10., ()), refreshing ? "🕓" : "")}
    {switch (data) {
      | None => str("⌛ loading...")
      | Some((
          person,
          revisions,
        )) =>
    <scrollView
      layout={Layout.style(
        /* ~flexGrow=1., */
        ~alignItems=AlignStretch,
        ~alignSelf=AlignStretch,
        ~overflow=Scroll,
        (),
      )}
    >
      <view layout={Layout.style(~alignItems=AlignStretch, ())}>
        <view layout={Layout.style(~alignItems=AlignStretch, ())}>
          <revisionList title="✅ Ready to land" 
          snoozeItem
          revisions={revisions.mine.accepted} />
          <revisionList
            title="❌ Ready to update"
            snoozeItem
            revisions={revisions.mine.needsRevision}
          />
          <revisionList
            title="🙏 Ready to review"
            snoozeItem
            revisions={revisions.theirs.needsReview}
          />
          <revisionList title="⌛ Waiting on review"
            snoozeItem
            revisions={revisions.mine.needsReview}
          />
          <revisionList title="⌛❌ Waiting for them to change"
            snoozeItem
            revisions={revisions.theirs.needsRevision}
          />
          <revisionList title="⌛✅ Waiting for them to land"
            snoozeItem
            revisions={revisions.theirs.accepted}
          />
        </view>
      </view>
    </scrollView>
         }}
  </view>;
};

let run = assetsDir => {
  Fluid.App.launch(
    ~isAccessory=true,
    () => {
    Fluid.App.setupAppMenu(
      ~title="CodeReviewChecker",
      ~appItems=[||],
      ~menus=[| Fluid.App.defaultEditMenu() |]
    );

    let closeWindow = ref(() => ());

    let statusBarItem = ref(None);

    let refresh = ref(() => ());

    let win = Fluid.launchWindow(
      ~title="CodeReviewChecker",
      ~floating=true,
      ~hidden=true,
      ~onResize=({width, height}, window) => {
        window->Fluid.Window.resize({x: width, y: height})
      },
      ~onBlur=win => {
        Fluid.Window.hide(win);
      },
      <main
        assetsDir
        refresh
        setTitle={title => switch (statusBarItem^) {
          | None => ()
          | Some(item) => Fluid.App.statusBarSetTitle(item, title)
        }}
      />
    );

    closeWindow := () => Fluid.Window.hide(win);

    statusBarItem := Some(Fluid.App.statusBarItem(
      ~isVariableLength=true,
      ~title=String("⌛"),
      ~onClick=pos => {
        refresh^();
        Fluid.Window.showAtPos(win, pos)
      }
    ));
  });

};

module GitHub = GitHub;