
/**
 * @file
 * Provides JS behaviors
 */


Drupal.behaviors.MMNode = function (context) {
  // Hide the title form on page load
  if ($("input:radio[name='steps[1][settings][storage][node][node_title_options]']").val() != 'title') {
    $('#mm_node_title_default').hide();
  }
  else {
    $('#mm_node_title_default').show().addClass('open');
  }

  // Bind to the selection
  $("input:radio[name='steps[1][settings][storage][node][node_title_options]']").bind('change', function () {
    if ($(this).val() == 'title' || $(this).val() == 'default') {
      $('#mm_node_title_default').show('slow').addClass('open');
	}
    else {
      // If the title form is open, close it
      if ($('#mm_node_title_default').hasClass('open')) {
        $('#mm_node_title_default').hide('slow').removeClass('open');
      }
    }
  });

}