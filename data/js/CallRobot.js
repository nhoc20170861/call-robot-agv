if (!!window.EventSource) {
    var source = new EventSource("/events");

    source.addEventListener(
        "open",
        function (e) {
            console.log("Events Connected");
        },
        false
    );
    source.addEventListener(
        "error",
        function (e) {
            if (e.target.readyState != EventSource.OPEN) {
                console.log("Events Disconnected");
            }
        },
        false
    );

    source.addEventListener(
        "stateStationESP",
        function (e) {
            $("#state-station").text(e.data);
            console.log("stateStation", e.data);
        },
        false
    );

    source.addEventListener(
        "callRobotRun",
        function (e) {
            $("#notify-json").text(JSON.stringify(e.data, null, 4));
            console.log("message", e.data);
        },
        false
    );

    source.addEventListener(
        "httpGetURL",
        function (e) {
            const obj = JSON.parse(e.data);
            $("#notify-json").html(JSON.stringify(obj, null, 4));
            console.log("message", e.data);
        },
        false
    );
}
$(document).ready(function () {
    $("form").each(function () {
        var form = $(this);
        form.submit(function (e) {
            var actionUrl = form.attr("action");
            var method = form.attr("method");
            console.log(method);

            console.log(form.serialize());
            $.ajax({
                type: method,
                url: actionUrl,
                data: form.serialize(), // serializes the form's elements.
                success: function (data) {
                    const LineName = $("#LineName").val();
                    const StationName = $("#StationName").val();
                    // console.log(LineName);
                    // console.log(StationName);
                    $("#station-name").html(StationName);
                    $("#line-name").html(LineName);
                    $("#notify-json").html(JSON.stringify(data, null, 4));
                },
            });
            e.preventDefault();
        });
    });
    $(".button1").click(function () {
        $(".button1").prop("disabled", true);
        $.post("/getIdRobot", {}, function (response) {
            const myTimeout = setTimeout(function () {
                $(".button1").prop("disabled", false);
                clearTimeout(myTimeout)
            }, 2000);
            //const obj = JSON.parse(response);
        });
    });

    $(".button2").click(function () {
        $(".button2").prop("disabled", true);
        console.log("btn2 click!");
        $.post("/callRobotRun", {}, function (response) {
            console.log(response);
            const myTimeout = setTimeout(function () {
                $(".button2").prop("disabled", false);
                clearTimeout(myTimeout)
            }, 2000);
        });
    });


    const modal = $('.modal');
    const btn_reset = $('.btn_reset');
    const span_modal = $('.submit-reset');

    btn_reset.click(function () {
        // $(".btn_reset").prop("disabled", true);
        console.log("btn_reset click!");
        modal.show();
    });

    span_modal.click(function () {
        $.post("/resetStateStation", {}, function (response) {
            console.log(response);
            modal.hide();

        });
    });

    $(window).on('click', function (e) {
        if ($(e.target).is('.modal')) {

            modal.hide();
        }
    });

});
