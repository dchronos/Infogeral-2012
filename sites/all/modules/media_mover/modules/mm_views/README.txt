
Media Mover Views Module
===============================================
This enables some level of of views functionality for Media mover

Installation
===============================================

Here is an example view that you can use with the Media Mover views module.
While this is very simple it gives you and idea of what you need to output
in order to get the files you want.

$view = new view;
$view->name = 'view_of_files';
$view->description = 'View of files';
$view->tag = '';
$view->view_php = '';
$view->base_table = 'files';
$view->is_cacheable = FALSE;
$view->api_version = 2;
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('fields', array(
  'filepath' => array(
    'label' => 'Path',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'link_to_file' => 0,
    'exclude' => 0,
    'id' => 'filepath',
    'table' => 'files',
    'field' => 'filepath',
    'relationship' => 'none',
  ),
));
$handler->override_option('filters', array(
  'filepath' => array(
    'operator' => '!=',
    'value' => '',
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'case' => 1,
    'id' => 'filepath',
    'table' => 'files',
    'field' => 'filepath',
    'relationship' => 'none',
  ),
  'timestamp' => array(
    'operator' => '>',
    'value' => array(
      'type' => 'offset',
      'value' => '-1000 days',
      'min' => '',
      'max' => '',
    ),
    'group' => '0',
    'exposed' => FALSE,
    'expose' => array(
      'operator' => FALSE,
      'label' => '',
    ),
    'id' => 'timestamp',
    'table' => 'files',
    'field' => 'timestamp',
    'relationship' => 'none',
  ),
));
$handler->override_option('access', array(
  'type' => 'none',
));
$handler->override_option('cache', array(
  'type' => 'none',
));
$handler->override_option('items_per_page', 0);
