#!/usr/bin/env ruby

require "rexml/document"
require "fileutils"
include REXML


INKSCAPE = '/usr/bin/inkscape'
#INKSCAPE = '/usr/bin/inkscape' # like this works for me, while using `which` inkscape hangs
SRC = "src/icons.svg"
PREFIX = "src/icons/scalable"

def chopSVG(icon)
	FileUtils.mkdir_p(icon[:dir]) unless File.exists?(icon[:dir])
	unless (File.exists?(icon[:file]) && !icon[:forcerender])
		FileUtils.cp(SRC,icon[:file]) 
		puts " >> #{icon[:name]}"
		cmd = "#{INKSCAPE} -g #{icon[:file]} --select #{icon[:id]} --verb=\"FitCanvasToSelection;EditInvertInAllLayers"
		cmd += ";EditDelete;EditSelectAll;SelectionUnGroup;SelectionUnGroup;SelectionUnGroup;StrokeToPath;FileVacuum"
		cmd += ";FileSave;FileQuit\""
		puts cmd
		system(cmd)
		#saving as plain SVG gets rid of the classes :/
		#cmd = "#{INKSCAPE} -f #{icon[:file]} -z --vacuum-defs -l #{icon[:file]} > /dev/null 2>&1"
		#system(cmd)
		svgcrop = Document.new(File.new(icon[:file], 'r'))
		svgcrop.root.each_element("//path") do |path|
			if path.attributes['style'].include? 'fill:#1a1a1a;'
				path.remove
			end
		end
    icon_f = File.new(icon[:file],'w+')
    icon_f.puts svgcrop
    icon_f.close
	else
		puts " -- #{icon[:name]} already exists"
	end
end #end of function


#main
# Open SVG file.
svg = Document.new(File.new(SRC, 'r'))

if (ARGV[0].nil?) #render all SVGs
  puts "Rendering from icons in #{SRC}"
	# Go through every layer.
	svg.root.each_element("/svg/g[@inkscape:groupmode='layer']") do |context| 
		context_name = context.attributes.get_attribute("inkscape:label").value  
		puts "Going through layer '" + context_name + "'"
		context.each_element("g") do |icon|
			puts "DEBUG #{icon.attributes.get_attribute('id')}"
			dir = "#{PREFIX}/#{context_name}"
			unless icon.attributes.get_attribute("inkscape:label") == nil
				icon_name = icon.attributes.get_attribute("inkscape:label").value
				chopSVG({	:name => icon_name,
									:id => icon.attributes.get_attribute("id"),
									:dir => dir,
									:file => "#{dir}/#{icon_name}-symbolic.svg"}) unless icon_name[0] == "#"
			end
		end
	end
  puts "\nrendered all SVGs"
else #only render the icons passed
  icons = ARGV
  ARGV.each do |icon_name|
  	icon = svg.root.elements["//g[@inkscape:label='#{icon_name}']"]
  	dir = "#{PREFIX}/#{icon.parent.attributes['inkscape:label']}"
		chopSVG({	:name => icon_name,
		 					:id => icon.attributes["id"],
		 					:dir => dir,
		 					:file => "#{dir}/#{icon_name}-symbolic.svg",
		 					:forcerender => true})
	end
  puts "\nrendered #{ARGV.length} icons"
end
