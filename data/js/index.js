//Function to add date and time of last update
function updateDateTime() {
    var currentdate = new Date();
    var datetime = currentdate.getDate() + "/"
        + (currentdate.getMonth() + 1) + "/"
        + currentdate.getFullYear() + " at "
        + currentdate.getHours() + ":"
        + currentdate.getMinutes() + ":"
        + currentdate.getSeconds();
    document.getElementById("update-time").innerHTML = datetime;
    console.log(datetime);
}

$(document).ready(function () {
    $('.btn_nav').click(function () {
        $('.items').toggleClass("show");
        $('ul li').toggleClass("hide");
    });
});
function replaceClass(elem, oldClass, newClass) {

    if (elem.hasClass(oldClass)) {
        elem.removeClass(oldClass);
    }
    elem.addClass(newClass);
}

const delay = (delayInms) => {
    return new Promise(resolve => setTimeout(resolve, delayInms));
}
