/*
 *  For using flex boxes in column mode with views.
 *  To use with views set 2 classes with the form:
 *     "asu-webspark-views-column-height columns-\d+"
 *     e.g: "asu-webspark-views-column-height columns-2" for a 2 column display
 *
 * in a view at:
 *     /admin/structure/views/display/<VIEW NAME>/<VIEW DISPLAY NAME>/css_class
 *     e.g.: /admin/structure/views/display/biographies/panel_pane_1/css_class
 *
 * And set the class "column-item" on the rows at:
 *     /admin/structure/views/display/<VIEW NAME>/<VIEW DISPLAY NAME>/style_options
 *     e.g.: /admin/structure/views/nojs/display/biographies/panel_pane_1/style_options
 *
 * Note: The class "columns-\d+" only needs to be on an HTML ancestor of the HTML
 *       elements with the class "column-item" because the code will work out
 *       the actual parent of the items and set the actual parent to be the
 *       flex box in css.
 *
 * On the flexbox object the code will look for the necessary flexbox CSS and
 * if it's not there it will add the values. I.e.:
 *    display: flex
 *    flex-direction: column
 *    flex-wrap: wrap
 */
(function ($, Drupal) {
    Drupal.behaviors.websparkExtrasFlexColumnHeight = {
        attach: function (context, settings) {
            $(window).load(function() {
                ideaEnterpriseThemeSetFlexColHeight($, Drupal);  // Run on page load
            });
            $(window).resize(function() {
                ideaEnterpriseThemeSetFlexColHeight($, Drupal);  // Run on page resize
            });
        }
    };
})(jQuery, Drupal);

function ideaEnterpriseThemeSetFlexColHeight($, Drupal) {
    $('*[class*="columns-"]').each(function() {
        var columns = Number($(this).attr('class').match(/columns-\d+/)[0].substring(8));

        // Get the flexboxes
        var index = 0;
        var visibility = [];
        var flexboxes = $('');
        $(this).find('*[class*="column-item"]').parent().each(function() {
            flexboxes = $(flexboxes).add(this);   // Each parent is a flexbox
            visibility[index] = $(this).is(':visible');
            if (visibility[index]) {
                $(this).css('display', 'hidden'); // Hide them all as they may affect each other.
            }
            index++;
        });

        $(flexboxes).each(function() {
            var items = $(this).find('*[class*="column-item"]');
            var itemCount = $(items).length;
            if (columns > 1 && itemCount > 1) {
                if (columns > itemCount) {
                    columns = itemCount;
                }

                // We can't get the size if it's hidden so unhide it.
                $(this).css('display', '');

                // Make sure the flexbox has the CSS necessary to work
                var flexbox = $(items).parent();
                if ($(flexbox).css('display') !== 'flex') {
                    $(flexbox).css('display', 'flex');
                }
                if ($(flexbox).css('justify-content') === 'normal') {
                    $(flexbox).css('justify-content', 'space-between');
                }

                if ($(flexbox).css('flex-direction') === 'row') { // "row" is the default so if it's "row" it's probably not set.
                    $(flexbox).css('flex-direction', 'column');
                }
                if ($(flexbox).css('flex-wrap') === 'nowrap') {   // "nowrap" is the default so if it's "nowrap" it's probably not set.
                    $(flexbox).css('flex-wrap', 'wrap');
                }

                // Get the combined height of all the items
                var itemHeight = 0;
                var itemsHeight = 0;
                var itemHighest = 0;
                $(items).each(function() {
                    itemHeight = Number($(this)[0].scrollHeight);
                    itemsHeight += itemHeight;
                    if (itemHeight > itemHighest) {
                        itemHighest = itemHeight;
                    }
                });
                var itemsPerColumn = itemCount / columns;
                var averageItemHeight = Math.floor(itemsHeight / itemCount);
                var flexBottomGap = 20;
                var flexHeight = Math.max(averageItemHeight * Math.ceil(itemsPerColumn), itemHighest) + flexBottomGap;
                // This is not a perfect height, it's a starting point.
                $(flexbox).css('max-height', flexHeight);

                // If there wasn't enough room then there will be too many columns.
                // In this case we add a bit to the height until we have the right
                // number of columns.
                var itemsLeft = {};
                var length = 0;
                var iterations = 0;
                var iterationsMax = 50; // A failsafe so we can't get an infinite loop
                do {
                    if (length > columns) {
                        itemsLeft = {};
                        flexHeight = flexHeight + flexBottomGap;
                        $(flexbox).css('max-height', flexHeight);
                    }
                    $(items).each(function() {
                        var left = Math.ceil(Number($(this).offset().left));
                        itemsLeft[left] = true;
                    });
                    length = Object.keys(itemsLeft).length;
                    iterations++;
                }
                while (length > columns && iterations < iterationsMax);

                // This fixes the amount of space at the bottom of the flexbox so
                // there is not too much or little room at the bottom.
                var itemBottomMax = 0;
                $(items).each(function() {
                    var itemBottom = Number($(this).offset().top) + Number($(this)[0].scrollHeight);
                    if (itemBottom > itemBottomMax) {
                        itemBottomMax = itemBottom;
                    }
                });
                var flexBottom = Number($(this).offset().top) + Number($(this)[0].scrollHeight);
                var flexHeightAdj = itemBottomMax - flexBottom + flexBottomGap;
                $(flexbox).css('max-height', flexHeight + flexHeightAdj);

                // Hide it so we can do the next one without interference
                $(this).css('display', 'none');
            }
        });

        // Go through all the flexboxes and show/hide them as they were before we
        // started this function.
        index = 0;
        $(flexboxes).each(function() {
            if (visibility[index]) {
                $(this).css('display', 'flex');
            }
            index++;
        });
    });
};
