(function(e){Drupal.toggleFieldset=function(t){var n=e(t);if(n.is(".collapsed")){var r=e("> .fieldset-wrapper",t).hide();n.removeClass("collapsed").trigger({type:"collapsed",value:!1}).find("> legend span.fieldset-legend-prefix").html(Drupal.t("Hide")),r.slideDown({duration:"fast",easing:"linear",complete:function(){Drupal.collapseScrollIntoView(t),t.animating=!1},step:function(){Drupal.collapseScrollIntoView(t)}})}else n.trigger({type:"collapsed",value:!0}),e("> .fieldset-wrapper",t).slideUp("fast",function(){n.addClass("collapsed").find("> legend span.fieldset-legend-prefix").html(Drupal.t("Show")),t.animating=!1})},Drupal.collapseScrollIntoView=function(t){var n=document.documentElement.clientHeight||document.body.clientHeight||0,r=document.documentElement.scrollTop||document.body.scrollTop||0,i=e(t).offset().top,s=55;i+t.offsetHeight+s>n+r&&(t.offsetHeight>n?window.scrollTo(0,i):window.scrollTo(0,i+t.offsetHeight-n+s))},Drupal.behaviors.collapse={attach:function(t,n){e("fieldset.collapsible",t).once("collapse",function(){var t=e(this),n=location.hash&&location.hash!="#"?", "+location.hash:"";t.find(".error"+n).length&&t.removeClass("collapsed");var r=e('<span class="summary"></span>');t.bind("summaryUpdated",function(){var n=e.trim(t.drupalGetSummary());r.html(n?" ("+n+")":"")}).trigger("summaryUpdated");var i=e("> legend .fieldset-legend",this);e('<span class="fieldset-legend-prefix element-invisible"></span>').append(t.hasClass("collapsed")?Drupal.t("Show"):Drupal.t("Hide")).prependTo(i).after(" ");var s=e('<a class="fieldset-title" href="#"></a>').prepend(i.contents()).appendTo(i).click(function(){var e=t.get(0);return e.animating||(e.animating=!0,Drupal.toggleFieldset(e)),!1});i.append(r)})}}})(jQuery);