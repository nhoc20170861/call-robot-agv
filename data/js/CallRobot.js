if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
        console.log("Events Connected");
    }, false);
    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);

    source.addEventListener('stateStationESP', function (e) {
        $("#state-station").html(JSON.stringify(e.data, null, 4));
        console.log("stateStation", e.data);
    }, false);

    source.addEventListener('callRobotRun', function (e) {
        $("#notify-json").html(JSON.stringify(e.data, null, 4));
        // console.log("message", e.data);
    }, false);
    // source.addEventListener('sateStation', function (e) {
    //     $("#state-Station").html(JSON.stringify(e.data, null, 4));
    //     console.log("stateStation", e.data);
    // }, false);

    source.addEventListener('httpGetURL', function (e) {
        const obj = JSON.parse(e.data);
        $("#notify-json").html(JSON.stringify(obj, null, 4));
        console.log("message", e.data);

    }, false);


}
$(document).ready(function () {
    $("form").each(function () {
        var form = $(this);
        form.submit(function (e) {

            var actionUrl = form.attr('action');
            var method = form.attr('method');
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
                }
            });
            e.preventDefault();

        });
    });
    $(".button1").click(function () {
        $('.button1').prop('disabled', true);
        $.post("/getIdRobot", {}, function (response) {


            setTimeout(function () {
                $('.button1').prop('disabled', false);
            }, 2000);
            //const obj = JSON.parse(response);
        });
    });

    $(".button2").click(function () {
        $('.button2').prop('disabled', true);
        $.post("/callRobotRun", {}, function (response) {
            console.log(response);
            setTimeout(function () {
                $('.button2').prop('disabled', false);
            }, 2000)
        });
    });
    // this is the id of the form
});